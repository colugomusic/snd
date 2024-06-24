#pragma once

namespace snd {

[[nodiscard]] inline auto is_flag_set(int mask, int flag) -> bool { return bool(mask & flag); } 
[[nodiscard]] inline auto set_flag(int mask, int flag) -> int { return (mask |= flag); }
[[nodiscard]] inline auto unset_flag(int mask, int flag) -> int { return (mask &= ~(flag)); }
[[nodiscard]] inline auto set_flag(int mask, int flag, bool on) -> int { return on ? set_flag(mask, flag) : unset_flag(mask, flag); } 
[[nodiscard]] inline auto is_flag_different(int a, int b, int flag) -> bool { return is_flag_set(a, flag) != is_flag_set(b, flag); }

template <typename Mask> [[nodiscard]]
auto has_single_bit(Mask mask) -> bool {
	return mask.value != 0 && (mask.value & (mask.value - 1)) == 0;
}

template <typename Mask> [[nodiscard]]
auto is_flag_set(Mask mask, typename Mask::e flag) -> bool {
	return bool(mask.value & flag);
}

template <typename Mask> [[nodiscard]]
auto set_flag(Mask mask, typename Mask::e flag) -> Mask {
	return {(mask.value |= flag)};
}

template <typename Mask> [[nodiscard]]
auto unset_flag(Mask mask, typename Mask::e flag) -> Mask {
	return {(mask.value &= ~(flag))};
} 

template <typename Mask> [[nodiscard]]
auto set_flag(Mask mask, typename Mask::e flag, bool on) -> Mask {
	return on ? set_flag(mask, flag) : unset_flag(mask, flag);
} 

template <typename Mask> [[nodiscard]]
auto is_flag_different(Mask a, Mask b, typename Mask::e flag) -> bool {
	return is_flag_set(a, flag) != is_flag_set(b, flag);
}

} // snd
