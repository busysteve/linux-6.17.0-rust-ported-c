// SPDX-License-Identifier: GPL-2.0

/*
 * Nova Core GPU Driver - Utility functions
 *
 * C port of Rust nova-core utility module
 * Original Rust version: drivers/gpu/nova-core/util.rs
 */

#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/ctype.h>

/**
 * to_lowercase_bytes - Convert string to lowercase bytes
 * @s: Input string
 * @dst: Destination buffer
 * @n: Maximum length to convert
 *
 * Convert input string to lowercase and store in destination buffer.
 */
void to_lowercase_bytes(const char *s, u8 *dst, size_t n)
{
	size_t i;
	size_t len = strlen(s);

	for (i = 0; i < len && i < n; i++) {
		dst[i] = tolower(s[i]);
	}

	/* Pad remaining bytes with zeros */
	for (; i < n; i++) {
		dst[i] = 0;
	}
}

/**
 * const_bytes_to_str - Convert bytes to string (compile-time safe)
 * @bytes: Input byte array
 * @len: Length of byte array
 *
 * Returns: String representation of bytes if valid UTF-8, NULL otherwise
 */
const char *const_bytes_to_str(const u8 *bytes, size_t len)
{
	/* Simple ASCII validation - for UTF-8 would need more complex checks */
	size_t i;

	for (i = 0; i < len; i++) {
		if (bytes[i] > 127 && bytes[i] != 0) {
			/* Non-ASCII character found */
			return NULL;
		}
	}

	return (const char *)bytes;
}

/**
 * wait_on - Wait until condition is true or timeout elapsed
 * @timeout_ns: Timeout in nanoseconds
 * @cond: Condition function to check
 * @data: Data to pass to condition function
 *
 * Returns: 0 on success, -ETIMEDOUT on timeout
 */
int wait_on(u64 timeout_ns, int (*cond)(void *), void *data)
{
	u64 start_time = ktime_get_ns();
	u64 elapsed;

	while (1) {
		if (cond && cond(data))
			return 0;

		elapsed = ktime_get_ns() - start_time;
		if (elapsed > timeout_ns)
			return -ETIMEDOUT;

		/* Small delay to avoid busy waiting */
		usleep_range(1, 10);
	}
}

/**
 * wait_on_simple - Simple wait with boolean condition
 * @timeout_ms: Timeout in milliseconds
 * @condition: Pointer to boolean condition
 *
 * Returns: 0 on success, -ETIMEDOUT on timeout
 */
int wait_on_simple(unsigned int timeout_ms, bool *condition)
{
	unsigned long start_time = jiffies;
	unsigned long timeout_jiffies = msecs_to_jiffies(timeout_ms);

	while (!(*condition)) {
		if (time_after(jiffies, start_time + timeout_jiffies))
			return -ETIMEDOUT;

		msleep(1);
	}

	return 0;
}

/**
 * div_round_up - Divide and round up to nearest integer
 * @dividend: Number to divide
 * @divisor: Divisor
 *
 * Returns: Result of division rounded up
 */
static inline u64 div_round_up(u64 dividend, u64 divisor)
{
	return (dividend + divisor - 1) / divisor;
}

/**
 * align_up - Align value up to alignment boundary
 * @value: Value to align
 * @alignment: Alignment boundary (must be power of 2)
 *
 * Returns: Aligned value
 */
static inline u64 align_up(u64 value, u64 alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * align_down - Align value down to alignment boundary
 * @value: Value to align
 * @alignment: Alignment boundary (must be power of 2)
 *
 * Returns: Aligned value
 */
static inline u64 align_down(u64 value, u64 alignment)
{
	return value & ~(alignment - 1);
}

/**
 * is_power_of_2 - Check if value is power of 2
 * @value: Value to check
 *
 * Returns: true if power of 2, false otherwise
 */
static inline bool is_power_of_2(u64 value)
{
	return value && !(value & (value - 1));
}

/**
 * bit_mask - Create bit mask with specified number of bits
 * @bits: Number of bits in mask
 *
 * Returns: Bit mask with 'bits' number of 1s
 */
static inline u64 bit_mask(int bits)
{
	if (bits >= 64)
		return ~0ULL;
	if (bits <= 0)
		return 0;
	
	return (1ULL << bits) - 1;
}

/**
 * extract_bits - Extract bits from value
 * @value: Source value
 * @start_bit: Starting bit position
 * @num_bits: Number of bits to extract
 *
 * Returns: Extracted bits
 */
static inline u64 extract_bits(u64 value, int start_bit, int num_bits)
{
	return (value >> start_bit) & bit_mask(num_bits);
}

/**
 * set_bits - Set bits in value
 * @value: Target value
 * @start_bit: Starting bit position
 * @num_bits: Number of bits to set
 * @new_bits: New bit values
 *
 * Returns: Modified value
 */
static inline u64 set_bits(u64 value, int start_bit, int num_bits, u64 new_bits)
{
	u64 mask = bit_mask(num_bits) << start_bit;
	return (value & ~mask) | ((new_bits & bit_mask(num_bits)) << start_bit);
}

/**
 * min_t - Type-safe minimum
 * @type: Type of the values
 * @a: First value
 * @b: Second value
 *
 * Returns: Minimum of a and b
 */
#define min_t(type, a, b) ((type)(a) < (type)(b) ? (type)(a) : (type)(b))

/**
 * max_t - Type-safe maximum
 * @type: Type of the values
 * @a: First value
 * @b: Second value
 *
 * Returns: Maximum of a and b
 */
#define max_t(type, a, b) ((type)(a) > (type)(b) ? (type)(a) : (type)(b))

/**
 * clamp_t - Type-safe clamp
 * @type: Type of the values
 * @val: Value to clamp
 * @min_val: Minimum value
 * @max_val: Maximum value
 *
 * Returns: Clamped value
 */
#define clamp_t(type, val, min_val, max_val) \
	max_t(type, min_t(type, val, max_val), min_val)