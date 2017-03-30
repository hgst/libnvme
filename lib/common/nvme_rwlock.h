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

#ifndef __NVME_RWLOCK_H__
#define __NVME_RWLOCK_H__

#include "nvme_common.h"
#include "nvme_atomic.h"

/*
 * nvme_rwlock_t type.
 */
typedef struct {

	/*
	 * -1 when write lock is held, and > 0 when read locks are held.
	 */
	volatile int32_t cnt;

} nvme_rwlock_t;

/*
 * A static rwlock initializer.
 */
#define NVME_RWLOCK_INITIALIZER { 0 }

/*
 * Initialize the rwlock to an unlocked state.
 */
static inline void nvme_rwlock_init(nvme_rwlock_t *rwl)
{
	rwl->cnt = 0;
}

/*
 * Take a read lock. Loop until the lock is held.
 */
static inline void nvme_rwlock_read_lock(nvme_rwlock_t *rwl)
{
	int32_t x;
	int success = 0;

	while (success == 0) {
		x = rwl->cnt;
		/* write lock is held */
		if (x < 0) {
			nvme_pause();
			continue;
		}
		success = nvme_atomic32_cmpset((volatile uint32_t *)&rwl->cnt,
					       x, x + 1);
	}
}

/*
 * Release a read lock.
 */
static inline void nvme_rwlock_read_unlock(nvme_rwlock_t *rwl)
{
	nvme_atomic32_dec((nvme_atomic32_t *)(intptr_t)&rwl->cnt);
}

/*
 * Take a write lock. Loop until the lock is held.
 */
static inline void nvme_rwlock_write_lock(nvme_rwlock_t *rwl)
{
	int32_t x;
	int success = 0;

	while (success == 0) {
		x = rwl->cnt;
		/* a lock is held */
		if (x != 0) {
			nvme_pause();
			continue;
		}
		success = nvme_atomic32_cmpset((volatile uint32_t *)&rwl->cnt,
					       0, -1);
	}
}

/*
 * Release a write lock.
 */
static inline void nvme_rwlock_write_unlock(nvme_rwlock_t *rwl)
{
	nvme_atomic32_inc((nvme_atomic32_t *)(intptr_t)&rwl->cnt);
}

#endif /* __NVME_RWLOCK_H__ */
