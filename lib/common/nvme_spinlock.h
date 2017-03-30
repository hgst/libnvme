/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   Copyright (c) 2017, Western Digital Corporation or its affiliates.
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

#ifndef __NVME_SPINLOCK_H__
#define __NVME_SPINLOCK_H__

#include "nvme_common.h"

/*
 * The nvme_spinlock_t type.
 */
typedef struct {

	/*
	 * lock status 0 = unlocked, 1 = locked
	 */
	volatile int locked;

} nvme_spinlock_t;

/*
 * Spinlock initializer.
 */
#define NVME_SPINLOCK_INITIALIZER { 0 }

/*
 * Initialize the spinlock to an unlocked state.
 */
static inline void nvme_spinlock_init(nvme_spinlock_t *sl)
{
	sl->locked = 0;
}

/*
 * Take the spinlock.
 */
static inline void nvme_spin_lock(nvme_spinlock_t *sl)
{
	while (__sync_lock_test_and_set(&sl->locked, 1))
		while (sl->locked)
			nvme_pause();
}

/*
 * Release the spinlock.
 */
static inline void nvme_spin_unlock(nvme_spinlock_t *sl)
{
	__sync_lock_release(&sl->locked);
}

/*
 * Try to take the lock.
 * Returns 1 if the lock is successfully taken; 0 otherwise.
 */
static inline int nvme_spinlock_trylock(nvme_spinlock_t *sl)
{
	return __sync_lock_test_and_set(&sl->locked,1) == 0;
}

/*
 * Test if the lock is taken.
 * Returns 1 if the lock is currently taken; 0 otherwise.
 */
static inline int nvme_spinlock_is_locked(nvme_spinlock_t *sl)
{
	return sl->locked;
}

#endif /* __NVME_SPINLOCK_H__ */
