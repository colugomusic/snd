#pragma once

#include "audio_object_manager.h"

/*
* Lock-free synchronization tools.
* 
* Everything here assumes there is one GUI thread and one audio thread.
* 
* Basic concept is that when the GUI thread wants to modify an object in a way
* that needs to be synchronized, it instead creates a copy of the object and
* performs the modifications on the copy. It then calls gui().commit() to make the
* new version of the object visible to the audio thread.
* 
* The audio thread should call audio().pending() to check if there is a new version
* of the object waiting to be picked up, and audio().get_next() to retrieve the new
* version.
* 
* The object returned from gui() overloads the -> and * operators which can be used
* to access the most recently committed version of the object. This is the version
* which is used as a base by gui().make_copy().
* 
* The object returned from audio() also overloads the -> and * operators, and can
* be used to access the most recently retrieved version of the object (which is not
* necessarily the most recently committed).
* 
* audio()->get_recent() peeks the new version if there is one pending, or returns
* the most recently retrieved version if there is no new version pending.
* 
* usage:
* 
* AudioObject<Thing> thing;
* 
* in GUI thread:
* 
*	//
*	// initialization:
*	//
* 
*	thing.gui().commit(thing.gui().make_new());
* 
*	//	
*	// updating:
*	//
* 
*   // Copies the most recently commit()'ed version of the object
*	auto new_thing = thing.gui().make_copy(); 
*	
*	new_thing->a = 1;
*	new_thing->b = 2;
*	new_thing->c = 3;
*	
*	thing.gui().commit(new_thing);
* 
* in audio thread (1):
*	
*	if (thing.audio().pending())
*	{
*		thing.audio().get_next();
*	}
*		
*	thing.audio()->do_things();
* 
* in audio thread (2):
* 
*	// gets a new 
*	Thing* ptr;
* 
*	if (thing.audio().pending())
*	{
*		// resulting pointer must be explicitly disposed, otherwise
*		// there will be a memory leak
*		ptr = thing.audio().get_next_unmanaged();
*	}
* 
*	ptr->do_things();
*	
*	thing.dispose(ptr);
*/

namespace snd {
namespace threads {

template <class T> class AudioObject;

template <class T>
class AudioObjectSetup
{
	friend class AudioObject<T>;

	T* object_ = nullptr;

	AudioObjectSetup(T* source)
		: object_(new T(*source))
	{
	}

	template <class ...Args>
	AudioObjectSetup(Args ... args)
		: object_(new T(args...))
	{
	}

public:

	~AudioObjectSetup()
	{
		if (object_) delete object_;
	}

	T* operator->() { return object_; }
	T& operator*() { return *object_; }
};

/*
 * These AudioObjectAudio and AudioObjectGUI classes are simply used to create
 * a clear distinction between audio and GUI operations at the call site. There
 * is nothing stopping you from calling audio() in a GUI thread or gui() in an
 * audio thread, if you know what you're doing.
 */

template <class T>
class AudioObjectAudio
{
	friend class AudioObject<T>;

	AudioObject<T>* object_;
	T* current_ = nullptr;
	T* retrieved_ = nullptr;

	AudioObjectAudio(AudioObject<T>* object)
		: object_(object)
	{
	}

public:

	~AudioObjectAudio()
	{
		if (current_) object_->dispose(current_);
	}

	bool pending() const
	{
		return object_->pending();
	}

	T* get_next()
	{
		if (current_) object_->dispose(current_);

		current_ = object_->get_next();

		retrieved_ = current_;

		return current_;
	}

	T* get_next_unmanaged()
	{
		return (retrieved_ = object_->get_next());
	}

	// If there's a new version waiting, peek it without
	// retrieving it. Otherwise return the last retrieved
	// version
	T* get_recent()
	{
		if (pending())
		{
			return object_->next_;
		}

		return retrieved_;
	}

	T* operator->() { return retrieved_; }
	const T* operator->() const { return retrieved_; }
	T& operator*() { return *retrieved_; }
	const T& operator*() const { return *retrieved_; }
};

template <class T>
class AudioObjectGUI
{
	friend class AudioObject<T>;

	AudioObject<T>* object_;

	AudioObjectGUI(AudioObject<T>* object)
		: object_(object)
	{
	}

public:

	template <class ...Args>
	AudioObjectSetup<T> make_new(Args... args)
	{
		return object_->make_new(args...);
	}

	AudioObjectSetup<T> make_copy()
	{
		return object_->make_copy();
	}

	AudioObjectSetup<T> make_copy(T* source)
	{
		return object_->make_copy(source);
	}

	void commit(AudioObjectSetup<T>& setup)
	{
		object_->commit(setup);
	}

	T* operator->() { return object_->recent_; }
	const T* operator->() const { return object_->recent_; }
	T& operator*() { return *object_->recent_; }
	const T& operator*() const { return *object_->recent_; }

	operator bool() const { return object_->recent_; }
};

template <class T>
class AudioObject
{
	friend class AudioObjectAudio<T>;
	friend class AudioObjectGUI<T>;

	static AudioObjectManager<T> manager_;

	AudioObjectAudio<T> audio_interface_;
	AudioObjectGUI<T> gui_interface_;
	std::atomic<T*> next_ = nullptr;
	T* recent_ = nullptr;

	template <class ...Args>
	AudioObjectSetup<T> make_new(Args... args)
	{
		return AudioObjectSetup<T>(args...);
	}

	AudioObjectSetup<T> make_copy()
	{
		return AudioObjectSetup<T>(*recent_);
	}

	AudioObjectSetup<T> make_copy(T* source)
	{
		return AudioObjectSetup<T>(*source);
	}

	void commit(AudioObjectSetup<T>& setup)
	{
		recent_ = setup.object_;

		setup.object_ = nullptr;

		manager_.add(recent_);

		auto prev_next = next_.exchange(recent_);

		if (prev_next) manager_.dispose(prev_next);

		manager_.collect();
	}

	bool pending() const { return next_; }
	T* get_next() { return next_.exchange(nullptr); }

public:

	AudioObject()
		: audio_interface_(this)
		, gui_interface_(this)
	{
	}

	AudioObject& operator=(AudioObject&& rhs)
	{
		audio_interface_ = std::move(rhs.audio_interface_);

		next_.store(rhs.next_.load());
		recent_ = rhs.recent_;

		rhs.next_ = nullptr;
		rhs.recent_ = nullptr;

		return *this;
	}

	~AudioObject()
	{
		if (next_) manager_.dispose(next_);

		manager_.collect();
	}

	AudioObjectAudio<T>& audio() { return audio_interface_; }
	const AudioObjectAudio<T>& audio() const { return audio_interface_; }
	AudioObjectGUI<T>& gui() { return gui_interface_; }
	const AudioObjectGUI<T>& gui() const { return gui_interface_; }

	void dispose(T* object)
	{
		manager_.dispose(object);
	}
};

template <class T>
AudioObjectManager<T> AudioObject<T>::manager_;

}}