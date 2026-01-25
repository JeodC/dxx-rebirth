/*
 * This file is part of the DXX-Rebirth project.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace dcx {

/* If enable_bit_enum<lhs_type, rhs_type> is true, enable all the supported
 * bit-operations overloads.
 *
 * If it is false, selected operations may be enabled individually.
 */
template <typename lhs_type, typename rhs_type = lhs_type>
/* There is no constraint on `rhs_type` because `operator~()` returns a struct,
 * not an enum.
 */
requires(std::is_enum_v<lhs_type>)
inline constexpr bool enable_bit_enum{false};

/* If enable_bit_enum_bitnot<enum_type> is true, enable:
 * - `operator~(enum_type)`
 */
template <typename enum_type>
requires(std::is_enum_v<enum_type>)
inline constexpr bool enable_bit_enum_bitnot{enable_bit_enum<enum_type, enum_type>};

/* Compute the bitwise negation of the input enumerated value.  Return it
 * wrapped in a single member structure.
 */
template <typename enum_type>
requires(
	std::is_enum_v<enum_type> &&
	enable_bit_enum_bitnot<enum_type>
)
[[nodiscard]]
constexpr auto operator~(const enum_type e)
{
	/* Returning a bare `enum class` would be preferable, but an `enum`
	 * cannot carry additional type information.  Later logic needs access
	 * to the `enum_type` that was negated to produce this result, so
	 * return a structure containing a single data member, and a type to
	 * remember the original `enum_type`.
	 *
	 * <gcc-14 need the `static_cast<result::bitnot>`
	 * >=gcc-14 allow using a direct-list-initialization from
	 * `underlying_type_t<enum_type>`
	 *
	 * For the sake of <gcc-14, give the return struct a name, so that its enum
	 * can be referenced in the static_cast.
	 */
	struct result {
		/* gcc-14, gcc-15 warn if this typedef is not used in a given
		 * translation unit.  Uses in non-instantiated templates do not count.
		 * Therefore, mark it as conditionally unused, to avoid the warning
		 * when a translation unit uses `operator~`, but does not use any of
		 * the functions that consume the type alias.
		 */
		using original_type [[maybe_unused]] = enum_type;
		enum class bitnot : std::underlying_type_t<enum_type>
		{
		} value;
	};
	return result{.value{static_cast<result::bitnot>(static_cast<std::underlying_type_t<enum_type>>(~(static_cast<std::underlying_type_t<enum_type>>(e))))}};
}

/* Take the result of the above bitwise negation, apply a bitwise negate, and
 * return the bare enum type. */
template <typename T>
requires(
	requires {
		typename T::original_type;
		requires enable_bit_enum_bitnot<typename T::original_type>;
		operator~(std::declval<typename T::original_type>());
	}
)
[[nodiscard]]
constexpr auto operator~(const T t)
{
	/* In the unlikely case that a caller writes `~~e`, return the original
	 * type of `e`, not a doubly-wrapped type.
	 */
	return static_cast<typename T::original_type>(~(static_cast<std::underlying_type_t<typename T::original_type>>(t.value)));
}

/* If enable_bit_enum_boolnot<enum_type> is true, enable:
 * - `operator!(enum_type)`
 * - `operator+(enum_type)`
 */
template <typename enum_type>
requires(std::is_enum_v<enum_type>)
inline constexpr bool enable_bit_enum_boolnot{enable_bit_enum<enum_type, enum_type>};

/* Compute the boolean negation of the input enumerated value.
 */
template <typename enum_type>
requires(
	std::is_enum_v<enum_type> &&
	enable_bit_enum_boolnot<enum_type>
)
[[nodiscard]]
constexpr auto operator!(const enum_type e)
{
	return !static_cast<std::underlying_type_t<enum_type>>(e);
}

/* Convert the enum_type to its underlying integer value.  This allows callers
 * to use the enum in a truth expression, without requiring the compiler to
 * convert all non-zero values to `true`.  The optimizer would normally delete
 * such a conversion if the resulting use was immediate, but would not if the
 * conversion was passed down and used later.  With this enabled, callers can
 * write:
 *
 *	if (+(enum_value & flag)) { ... }
 *	f(+(enum_value & flag));
 *
 * Without this, such callers would need to use:
 *
 *	if (!!(enum_value & flag)) { ... }
 *	f(!!(enum_value & flag));
 *
 * The optimizer will likely generate the same code for both `if` statements.
 * However, for the function call, the latter would get rewritten as
 * `f(((enum_value & flag) != enum_type{}) ? true : false);` even if `f` can
 * treat all non-zero values as true.
 */
template <typename enum_type>
requires(
	std::is_enum_v<enum_type> &&
	/* Use enable_bit_enum_boolnot, rather than a dedicated guard, since types
	 * that want to boolean-negate an enum usually want to be able to get the
	 * underlying type as a truth value.
	 */
	enable_bit_enum_boolnot<enum_type>
)
[[nodiscard]]
constexpr auto operator+(const enum_type e)
{
	return static_cast<std::underlying_type_t<enum_type>>(e);
}

/* If enable_bit_enum_and<lhs_type, rhs_type> is true, enable:
 * - `operator&(lhs_type, rhs_type)`
 * - `operator&=(lhs_type &, rhs_type)`
 */
template <typename lhs_type, typename rhs_type>
requires(std::is_enum_v<lhs_type>)
inline constexpr bool enable_bit_enum_and{
	enable_bit_enum<lhs_type, rhs_type> ||
		(
			/* If `rhs_type` is the result of `operator~(other_type)`, then
			 * delegate to `lhs & other_type`.
			 */
			requires {
				typename rhs_type::original_type;
				requires enable_bit_enum_bitnot<typename rhs_type::original_type>;
				requires enable_bit_enum_and<lhs_type, typename rhs_type::original_type>;
			}
		)
};

/* Apply normal bit-and semantics to the underlying integer type, then cast it
 * back to enum type.
 */
template <typename lhs_type, typename rhs_type>
requires(
	std::is_enum_v<lhs_type> &&
	std::is_enum_v<rhs_type> &&
	enable_bit_enum_and<lhs_type, rhs_type> &&
	requires(rhs_type rhs) {
		lhs_type{static_cast<std::underlying_type_t<rhs_type>>(rhs)};	/* require that a value of type `rhs_type` can be stored into an enum of type `lhs_type` without loss of precision */
	}
)
[[nodiscard]]
constexpr lhs_type operator&(const lhs_type lhs, const rhs_type rhs)
{
	return static_cast<lhs_type>(static_cast<std::underlying_type_t<lhs_type>>(lhs) & static_cast<std::underlying_type_t<rhs_type>>(rhs));
}

/* Allow a rhs_type from the bitwise negation overload above.  Extract the
 * wrapped value and apply normal bit-and semantics.
 */
template <typename lhs_type, typename rhs_type>
requires(
	std::is_enum_v<lhs_type> &&
	requires {
		typename rhs_type::original_type;
		requires enable_bit_enum_and<lhs_type, typename rhs_type::original_type>;
		{ lhs_type{} & (typename rhs_type::original_type{}) };
	}
)
[[nodiscard]]
constexpr lhs_type operator&(const lhs_type lhs, const rhs_type rhs)
{
	return static_cast<lhs_type>(static_cast<std::underlying_type_t<lhs_type>>(lhs) & static_cast<std::underlying_type_t<typename rhs_type::original_type>>(rhs.value));
}

template <typename lhs_type, typename rhs_type>
requires(
	std::is_enum_v<lhs_type> &&
	enable_bit_enum_and<lhs_type, rhs_type> &&
	requires(lhs_type lhs, rhs_type rhs) {
		{ lhs & rhs } -> std::same_as<lhs_type>;	/* require that bit-and be defined and produce `lhs_type`, so that the output can be assigned from the result of bit-and */
	}
)
constexpr lhs_type &operator&=(lhs_type &lhs, const rhs_type rhs)
{
	return lhs = lhs & rhs;
}

/* If enable_bit_enum_or<lhs_type, rhs_type> is true, enable:
 * - `operator|(lhs_type, rhs_type)`
 * - `operator|=(lhs_type &, rhs_type)`.
 */
template <typename lhs_type, typename rhs_type = lhs_type>
requires(
	std::is_enum_v<lhs_type> &&
	std::is_enum_v<rhs_type>
)
inline constexpr bool enable_bit_enum_or{enable_bit_enum<lhs_type, rhs_type>};

template <typename lhs_type, typename rhs_type>
requires(
	std::is_enum_v<lhs_type> &&
	std::is_enum_v<rhs_type> &&
	enable_bit_enum_or<lhs_type, rhs_type> &&
	requires(rhs_type rhs) {
		lhs_type{static_cast<std::underlying_type_t<rhs_type>>(rhs)};	/* require that a value of type `rhs_type` can be stored into an enum of type `lhs_type` without loss of precision */
	}
)
[[nodiscard]]
constexpr lhs_type operator|(const lhs_type lhs, const rhs_type rhs)
{
	return static_cast<lhs_type>(static_cast<std::underlying_type_t<lhs_type>>(lhs) | static_cast<std::underlying_type_t<rhs_type>>(rhs));
}

template <typename lhs_type, typename rhs_type>
requires(
	std::is_enum_v<lhs_type> &&
	std::is_enum_v<rhs_type> &&
	enable_bit_enum_or<lhs_type, rhs_type> &&
	requires(lhs_type lhs, rhs_type rhs) {
		{ lhs | rhs } -> std::same_as<lhs_type>;	/* require that bit-or be defined and produce `lhs_type` so that the result can be written back */
	}
)
constexpr lhs_type &operator|=(lhs_type &lhs, const rhs_type rhs)
{
	return lhs = (lhs | rhs);
}

#ifdef DXX_ENABLE_BIT_ENUM_COMPILE_TESTS
/* Everything in this #ifdef is related to performing compile-time
 * static_assert tests of the preceding code.
 */
namespace unit_test_detail
{

enum class e8_bitnot : std::uint8_t {};
enum class e8_boolnot : std::uint8_t {};
enum class e16_bitand_composite : std::uint16_t {};
enum class e16_bitand_flag : std::uint16_t {};
enum class e32_bitand_flag : std::uint32_t {};
enum class e16_bitor_composite : std::uint16_t {};
enum class e16_bitor_flag : std::uint16_t {};
enum class e32_bitor_flag : std::uint32_t {};

template <typename lhs_type>
concept has_bit_not = requires(lhs_type lhs) { ~lhs; };

template <typename lhs_type, typename rhs_type>
concept has_bit_and = requires(lhs_type lhs, rhs_type rhs) { lhs & rhs; };

template <typename lhs_type, typename rhs_type>
concept has_bit_and_assign = requires(lhs_type lhs, rhs_type rhs) { lhs &= rhs; };

template <typename lhs_type, typename rhs_type>
concept has_bit_or = requires(lhs_type lhs, rhs_type rhs) { lhs | rhs; };

template <typename lhs_type, typename rhs_type>
concept has_bit_or_assign = requires(lhs_type lhs, rhs_type rhs) { lhs |= rhs; };

}

template <>
inline constexpr bool enable_bit_enum_bitnot<unit_test_detail::e8_bitnot>{true};

template <>
inline constexpr bool enable_bit_enum_boolnot<unit_test_detail::e8_boolnot>{true};

template <>
inline constexpr bool enable_bit_enum_bitnot<unit_test_detail::e16_bitand_flag>{true};

static_assert(not std::same_as<unit_test_detail::e8_bitnot, decltype(~std::declval<unit_test_detail::e8_bitnot>())>, "bitnot should not return the original type");
static_assert(std::same_as<unit_test_detail::e8_bitnot, decltype(~~std::declval<unit_test_detail::e8_bitnot>())>, "double-bitnot should return the original type");
static_assert((~(unit_test_detail::e8_bitnot{2})).value == static_cast<typename decltype(~std::declval<unit_test_detail::e8_bitnot>())::bitnot>(253), "bitnot should produce the same bit pattern as using bitnot on the underlying type");
static_assert(~~(unit_test_detail::e8_bitnot{5}) == unit_test_detail::e8_bitnot{5}, "double-bitnot should produce the original input");
static_assert(std::same_as<bool, decltype(!std::declval<unit_test_detail::e8_boolnot>())>, "boolnot should return bool");
static_assert(!unit_test_detail::e8_boolnot{}, "boolnot should be true when the input is a zero value");
static_assert(!!unit_test_detail::e8_boolnot{1}, "boolnot should be false when the input is a non-zero value");
static_assert(std::same_as<std::underlying_type_t<unit_test_detail::e8_boolnot>, decltype(+std::declval<unit_test_detail::e8_boolnot>())>, "+enum should return underlying type");
static_assert(!+unit_test_detail::e8_boolnot{}, "+enum should be false when the input is a zero value");
static_assert(+unit_test_detail::e8_boolnot{1}, "+enum should be true when the input is a non-zero value");

template <>
inline constexpr bool enable_bit_enum_and<unit_test_detail::e16_bitand_composite, unit_test_detail::e16_bitand_flag>{true};

static_assert(not unit_test_detail::has_bit_and<unit_test_detail::e16_bitand_composite, unit_test_detail::e8_bitnot>, "bit-and should not be enabled for (e16_bitand_composite & e8_bitnot)");
static_assert(unit_test_detail::has_bit_and<unit_test_detail::e16_bitand_composite, unit_test_detail::e16_bitand_flag>, "bit-and should be enabled for (e16_bitand_composite & e16_bitand_flag)");
static_assert(unit_test_detail::has_bit_and<unit_test_detail::e16_bitand_composite, decltype(~std::declval<unit_test_detail::e16_bitand_flag>())>, "bit-and should be enabled for (e16_bitand_composite & ~e16_bitand_flag)");
static_assert(unit_test_detail::e16_bitand_composite{1} == (unit_test_detail::e16_bitand_composite{3} & unit_test_detail::e16_bitand_flag{1}), "bit-and should mask bits the same as underlying integer bit-and works");
static_assert(unit_test_detail::has_bit_and_assign<unit_test_detail::e16_bitand_composite, unit_test_detail::e16_bitand_flag>, "bit-and-assign should be enabled when bit-and is enabled");

template <>
inline constexpr bool enable_bit_enum_and<unit_test_detail::e16_bitand_composite, unit_test_detail::e32_bitand_flag>{true};

static_assert(not unit_test_detail::has_bit_and<unit_test_detail::e16_bitand_composite, unit_test_detail::e32_bitand_flag>, "bit-and should not be enabled for (e16_bitand_composite & e32_bitand_flag) due to narrowing");

template <>
inline constexpr bool enable_bit_enum_or<unit_test_detail::e16_bitor_composite, unit_test_detail::e16_bitor_flag>{true};

static_assert(not unit_test_detail::has_bit_or<unit_test_detail::e16_bitor_composite, unit_test_detail::e8_bitnot>, "bit-or should not be enabled for (e16_bitor_composite | e8_bitnot)");
static_assert(unit_test_detail::has_bit_or<unit_test_detail::e16_bitor_composite, unit_test_detail::e16_bitor_flag>, "bit-or should be enabled for (e16_bitor_composite & e16_bitor_flag)");
static_assert(not unit_test_detail::has_bit_or<unit_test_detail::e16_bitand_composite, decltype(~std::declval<unit_test_detail::e16_bitand_flag>())>, "bit-or should not be enabled for (e16_bitand_composite | ~e16_bitand_flag)");	/* Enabling `(a & ~b)` should not enable `(a | ~b)`, since bit-or with a negated mask is rarely useful. */
static_assert(unit_test_detail::e16_bitor_composite{3} == (unit_test_detail::e16_bitor_composite{2} | unit_test_detail::e16_bitor_flag{1}), "bit-or should combine bits the same as underlying integer bit-or works");
static_assert(unit_test_detail::has_bit_or_assign<unit_test_detail::e16_bitor_composite, unit_test_detail::e16_bitor_flag>, "bit-or-assign should be enabled when bit-or is enabled");

template <>
inline constexpr bool enable_bit_enum_or<unit_test_detail::e16_bitor_composite, unit_test_detail::e32_bitor_flag>{true};
static_assert(not unit_test_detail::has_bit_or<unit_test_detail::e16_bitor_composite, unit_test_detail::e32_bitor_flag>, "bit-or should not be enabled for (e16_bitor_composite & e32_bitor_flag) due to narrowing");
#endif

}
