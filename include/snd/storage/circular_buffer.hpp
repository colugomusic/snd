#pragma once

#include <array>
#include <vector>

namespace snd {
namespace storage {

template <class T, class Allocator = std::allocator<T>>
class CircularBuffer
{
public:

	using Data = std::vector<T, Allocator>;

	template <bool Const = false>
	class iterator
	{
	public:
		using self_type = iterator;
		using value_type = T;
		using data_type = typename std::conditional_t<Const, const Data*, Data*>;
		using reference = typename std::conditional_t<Const, T const&, T&>;
		using pointer = typename std::conditional_t<Const, T const*, T*>;
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::int32_t;

		iterator(data_type data, typename Data::size_type pos) : data_(data), pos_(difference_type(pos)) {}

		self_type operator++()
		{
			pos_++;

			if (pos_ >= data_->size()) pos_ -= difference_type(data_->size());

			return *this;
		}

		self_type operator++(int)
		{
			self_type result(*this);
			
			++(*this);

			return result;
		}

		self_type operator--()
		{
			pos_--;

			if (pos_ < 0) pos_ += difference_type(data_->size());

			return *this;
		}

		self_type operator--(int)
		{
			self_type result(*this);

			--(*this);

			return result;
		}

		self_type operator+(difference_type n)
		{
			return self_type(data_, (pos_ + n) % data_->size());
		}

		self_type operator-(difference_type n)
		{
			const auto new_pos = (pos_ - n) % data_->size();

			if (new_pos < 0) new_pos += difference_type(data_->size());

			return self_type(data_, new_pos);
		}

		reference operator*() const
		{
			return data_->at(pos_);
		}

		pointer operator->() const
		{
			return &(data_->at(pos_));
		}

		template<bool _Const = Const>
		std::enable_if_t<!_Const, reference> operator*()
		{
			return data_->at(pos_);
		}

		template<bool _Const = Const>
		std::enable_if_t<!_Const, pointer> operator->()
		{
			return &(data_->at(pos_));
		}

		bool operator==(const self_type& rhs)
		{
			return data_ == rhs.data_ && pos_ == rhs.pos_;
		}

		bool operator!=(const self_type& rhs)
		{
			return data_ != rhs.data_ || pos_ != rhs.pos_;
		}

	private:

		data_type data_ = nullptr;
		difference_type pos_ = 0;
	};

	CircularBuffer() = default;

	CircularBuffer(typename Data::size_type size)
		: data_(size)
	{
	}

	void resize(typename Data::size_type size)
	{
		data_.resize(size);
	}

	typename Data::size_type size() const { return data_.size(); }

	T& operator[](typename Data::size_type idx) { return data_[idx % data_.size()]; }

	const T& at(typename Data::size_type idx) const { return data_.at(idx % data_.size()); }

	auto begin() { return iterator<false>(&data_, 0); }
	auto end() { return iterator<false>(&data_, 0); }

	auto begin() const { return iterator<true>(&data_, 0); }
	auto end() const { return iterator<true>(&data_, 0); }

private:

	Data data_;
};

template <class T, size_t ROWS, class Allocator = std::allocator<T>>
class CircularBufferArray
{
	using Buffers = std::array<CircularBuffer<T, Allocator>, ROWS>;

	Buffers buffers_;

public:

	CircularBufferArray() = default;

	CircularBufferArray(typename CircularBuffer<T>::Data::size_type size)
	{
		resize(size);
	}
	
	void resize(typename CircularBuffer<T>::Data::size_type size)
	{
		for (auto& buffer : buffers_)
		{
			buffer.resize(size);
		}
	}

	typename Buffers::size_type size() const { return buffers_[0].size(); }

	CircularBuffer<T, Allocator>& operator[](typename Buffers::size_type idx)
	{
		return buffers_[idx];
	}

	const CircularBuffer<T, Allocator>& operator[](typename Buffers::size_type idx) const
	{
		return buffers_.at(idx);
	}

	template <int Row, class InputIt>
	bool write(InputIt first, InputIt last, typename CircularBuffer<T>::Data::size_type pos)
	{
		return buffers_[Row].write(first, last, pos);
	}
};

}}