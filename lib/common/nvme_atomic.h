/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __NVME_ATOMIC_H__
#define __NVME_ATOMIC_H__

#include "nvme_common.h"

#include <stdint.h>

/*
 * 32 bits atomic counter structure.
 */
typedef struct {
	volatile int32_t cnt;
} nvme_atomic_t;

/*
 * Static initializer for an atomic counter.
 */
#define NVME_ATOMIC_INIT(val)	{ (val) }

/*
 * Initialize an atomic counter.
 */
static inline void nvme_atomic_init(nvme_atomic_t *v)
{
	v->cnt = 0;
}

/*
 * Atomically read a value from a counter.
 */
static inline int32_t nvme_atomic_read(const nvme_atomic_t *v)
{
	return v->cnt;
}

/*
 * Atomically set a counter to a value.
 */
static inline void nvme_atomic_set(nvme_atomic_t *v,
				   int32_t new_value)
{
	v->cnt = new_value;
}

/*
 * Atomically add a value to an atomic counter.
 */
static inline void nvme_atomic_add(nvme_atomic_t *v, int32_t inc)
{
	__sync_fetch_and_add(&v->cnt, inc);
}

/*
 * Atomically subtract a value from an atomic counter.
 */
static inline void nvme_atomic_sub(nvme_atomic_t *v, int32_t dec)
{
	__sync_fetch_and_sub(&v->cnt, dec);
}

/*
 * Atomically increment a counter by one.
 */
static inline void nvme_atomic_inc(nvme_atomic_t *v)
{
	nvme_atomic_add(v, 1);
}

/*
 * Atomically decrement a counter by one.
 */
static inline void nvme_atomic_dec(nvme_atomic_t *v)
{
	nvme_atomic_sub(v,1);
}

/*
 * Atomically add a value to a counter and return the result.
 */
static inline int32_t nvme_atomic_add_return(nvme_atomic_t *v, int32_t inc)
{
	return __sync_add_and_fetch(&v->cnt, inc);
}

/*
 * Atomically subtracts a value from the atomic counter
 * and returns the value after the subtraction.
 */
static inline int32_t nvme_atomic_sub_return(nvme_atomic_t *v, int32_t dec)
{
	return __sync_sub_and_fetch(&v->cnt, dec);
}

/*
 * Atomically increment the atomic by one and returns true if
 * the result is 0, or false in all other cases.
 */
static inline int nvme_atomic_inc_and_test(nvme_atomic_t *v)
{
	return __sync_add_and_fetch(&v->cnt, 1) == 0;
}

/*
 * Atomically decrements the atomic by one and returns true if
 * the result is 0, or false in all other cases.
 */
static inline int nvme_atomic_dec_and_test(nvme_atomic_t *v)
{
	return __sync_sub_and_fetch(&v->cnt, 1) == 0;
}

/*
 * Atomically test and set a 32-bit atomic counter.
 * If the counter value is already set, return 0 (failed). Otherwise, set
 * the counter value to 1 and return 1 (success).
 */
static inline int nvme_atomic_test_and_set(nvme_atomic_t *v)
{
	return __sync_bool_compare_and_swap((volatile uint32_t *)&v->cnt, 0, 1);
}

/*
 * Atomically set a counter to 0.
 */
static inline void nvme_atomic_clear(nvme_atomic_t *v)
{
	v->cnt = 0;
}

/*
 * The atomic counter structure.
 */
typedef struct {
	volatile int64_t cnt;
} nvme_atomic64_t;

/*
 * Static initializer for an atomic counter.
 */
#define NVME_ATOMIC64_INIT(val) { (val) }

/*
 * Initialize an atomic counter.
 */
static inline void nvme_atomic64_init(nvme_atomic64_t *v)
{
#ifdef __LP64__
	v->cnt = 0;
#else
	int success = 0;
	uint64_t tmp;

	while (success == 0) {
		tmp = v->cnt;
		success = __sync_bool_compare_and_swap((volatile uint64_t *)&v->cnt, tmp, 0);
	}
#endif
}

/*
 * Atomically read a 64-bit counter.
 */
static inline int64_t nvme_atomic64_read(nvme_atomic64_t *v)
{
#ifdef __LP64__
	return v->cnt;
#else
	int success = 0;
	uint64_t tmp;

	while (success == 0) {
		tmp = v->cnt;
		/* replace the value by itself */
		success  = __sync_bool_compare_and_swap((volatile uint64_t *)&v->cnt,
							tmp, tmp);
	}
	return tmp;
#endif
}

/*
 * Atomically set a 64-bit counter.
 */
static inline void nvme_atomic64_set(nvme_atomic64_t *v, int64_t new_value)
{
#ifdef __LP64__
	v->cnt = new_value;
#else
	int success = 0;
	uint64_t tmp;

	while (success == 0) {
		tmp = v->cnt;
		success = __sync_bool_compare_and_swap((volatile uint64_t *)&v->cnt,
						       tmp, new_value);
	}
#endif
}

/*
 * Atomically add a 64-bit value to a counter.
 */
static inline void nvme_atomic64_add(nvme_atomic64_t *v, int64_t inc)
{
	__sync_fetch_and_add(&v->cnt, inc);
}

/*
 * Atomically subtract a 64-bit value from a counter.
 */
static inline void nvme_atomic64_sub(nvme_atomic64_t *v, int64_t dec)
{
	__sync_fetch_and_sub(&v->cnt, dec);
}

/*
 * Atomically increment a 64-bit counter by one and test.
 */
static inline void nvme_atomic64_inc(nvme_atomic64_t *v)
{
	nvme_atomic64_add(v, 1);
}

/*
 * Atomically decrement a 64-bit counter by one and test.
 */
static inline void nvme_atomic64_dec(nvme_atomic64_t *v)
{
	nvme_atomic64_sub(v, 1);
}

/*
 * Add a 64-bit value to an atomic counter and return the result.
 * Atomically adds the 64-bit value (inc) to the atomic counter (v) and
 * returns the value of v after the addition.
 */
static inline int64_t nvme_atomic64_add_return(nvme_atomic64_t *v, int64_t inc)
{
	return __sync_add_and_fetch(&v->cnt, inc);
}

/*
 * Subtract a 64-bit value from an atomic counter and return the result.
 * Atomically subtracts the 64-bit value (dec) from the atomic counter (v)
 * and returns the value of v after the subtraction.
 */
static inline int64_t nvme_atomic64_sub_return(nvme_atomic64_t *v, int64_t dec)
{
	return __sync_sub_and_fetch(&v->cnt, dec);
}

/*
 * Atomically increment a 64-bit counter by one and test.
 * Atomically increments the atomic counter (v) by one and returns
 * true if the result is 0, or false in all other cases.
 */
static inline int nvme_atomic64_inc_and_test(nvme_atomic64_t *v)
{
	return nvme_atomic64_add_return(v, 1) == 0;
}

/*
 * Atomically decrement a 64-bit counter by one and test.
 * Atomically decrements the atomic counter (v) by one and returns true if
 * the result is 0, or false in all other cases.
 */
static inline int nvme_atomic64_dec_and_test(nvme_atomic64_t *v)
{
	return nvme_atomic64_sub_return(v, 1) == 0;
}

/*
 * Atomically test and set a 64-bit atomic counter.
 * If the counter value is already set, return 0 (failed). Otherwise, set
 * the counter value to 1 and return 1 (success).
 */
static inline int nvme_atomic64_test_and_set(nvme_atomic64_t *v)
{
	return __sync_bool_compare_and_swap((volatile uint64_t *)&v->cnt, 0, 1);
}

/*
 * Atomically set a 64-bit counter to 0.
 */
static inline void nvme_atomic64_clear(nvme_atomic64_t *v)
{
	nvme_atomic64_set(v, 0);
}

#endif /* __NVME_ATOMIC_H__ */
