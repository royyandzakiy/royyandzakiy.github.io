// Q15 fixed-point converter, compiled to wasm for the blog post.
// Freestanding C++23: no standard library, only clang's builtin headers.
#include <stdint.h>

namespace q15 {

using q15_t = int16_t;

inline constexpr float scale = 32768.0f;
inline constexpr q15_t lo    = INT16_MIN;
inline constexpr q15_t hi    = INT16_MAX;

[[nodiscard]] constexpr q15_t from_float(float x)
{
    // Round half away from zero, then saturate
    const float s = x * scale + (x >= 0 ? 0.5f : -0.5f);
    if (s > hi) {
        return hi;
    }
    if (s < lo) {
        return lo;
    }
    return static_cast<q15_t>(s);
}

[[nodiscard]] constexpr float to_float(q15_t q) { return q / scale; }

[[nodiscard]] constexpr uint16_t bits(q15_t q)
{
    return __builtin_bit_cast(uint16_t, q);
}

// Run by the compiler: the build fails if any of these break
static_assert(from_float(0.5f) == 16384);
static_assert(from_float(1.0f) == hi);
static_assert(from_float(-1.0f) == lo);
static_assert(from_float(-0.3f) == -9830);
static_assert(to_float(from_float(0.25f)) == 0.25f);
static_assert(bits(-1) == 0xFFFF);

} // namespace q15

extern "C" {

[[clang::export_name("to_q15")]]
int16_t to_q15(float x) { return q15::from_float(x); }

[[clang::export_name("from_q15")]]
float from_q15(int16_t q) { return q15::to_float(q); }

[[clang::export_name("q15_bits")]]
uint16_t q15_bits(int16_t q) { return q15::bits(q); }

}
