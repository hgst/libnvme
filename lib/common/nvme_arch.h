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

#ifndef __NVME_ARCH_H__
#define __NVME_ARCH_H__

#if defined(__x86_64__)

#define NVME_ARCH		"x86_64"
#define NVME_ARCH_X86_64	1
#define NVME_ARCH_64		1
#undef  NVME_ARCH_X86
#define NVME_CACHE_LINE_SIZE	64
#define NVME_MMIO_64BIT		1

#elif defined(__i386__)

#define NVME_ARCH		"x86"
#undef NVME_ARCH_X86_64
#undef NVME_ARCH_64
#define NVME_ARCH_X86		1
#define NVME_CACHE_LINE_SIZE	64
#undef NVME_MMIO_64BIT

#else

#error "Unsupported architecture type"

#endif

#ifndef asm
#define asm __asm__
#endif

/*
 * Compiler barrier.
 * Guarantees that operation reordering does not occur at compile time
 * for operations directly before and after the barrier.
 */
#define	nvme_compiler_barrier() do {		\
	__asm__ volatile ("" : : : "memory");	\
} while(0)

/*
 * General memory barrier.
 * Guarantees that the LOAD and STORE operations generated before the
 * barrier occur before the LOAD and STORE operations generated after.
 * This function is architecture dependent.
 */
#define nvme_mb()	__asm__ volatile("mfence" ::: "memory")

/*
 * Write memory barrier.
 * Guarantees that the STORE operations generated before the barrier
 * occur before the STORE operations generated after.
 * This function is architecture dependent.
 */
#define nvme_wmb()	__asm__ volatile("sfence" ::: "memory")

/*
 * Read memory barrier.
 * Guarantees that the LOAD operations generated before the barrier
 * occur before the LOAD operations generated after.
 * This function is architecture dependent.
 */
#define nvme_rmb()	__asm__ volatile("lfence" ::: "memory")

/*
 * General memory barrier between CPUs.
 * Guarantees that the LOAD and STORE operations that precede the
 * nvme_smp_mb() call are globally visible across the lcores
 * before the the LOAD and STORE operations that follows it.
 */
#define nvme_smp_mb()	nvme_mb()

/*
 * Write memory barrier between CPUs.
 * Guarantees that the STORE operations that precede the
 * nvme_smp_wmb() call are globally visible across the lcores
 * before the the STORE operations that follows it.
 */
#define nvme_smp_wmb()	nvme_compiler_barrier()

/*
 * Read memory barrier between CPUs.
 * Guarantees that the LOAD operations that precede the
 * nvme_smp_rmb() call are globally visible across the lcores
 * before the the LOAD operations that follows it.
 */
#define nvme_smp_rmb()	nvme_compiler_barrier()

/*
 * Get the number of cycles since boot from the default timer.
 */
static inline __u64 nvme_rdtsc(void)
{
	union {
		__u64 tsc_64;
		struct {
			__u32 lo_32;
			__u32 hi_32;
		};
	} tsc;

	asm volatile("rdtsc" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
	return tsc.tsc_64;
}

static inline __u32 nvme_mmio_read_4(const volatile __u32 *addr)
{
	return *addr;
}

static inline void nvme_mmio_write_4(volatile __u32 *addr, __u32 val)
{
	*addr = val;
}

static inline __u64 nvme_mmio_read_8(volatile __u64 *addr)
{
#ifdef NVME_MMIO_64BIT
		return *addr;
#else
	volatile __u32 *addr32 = (volatile __u32 *)addr;
	__u64 val;

	/*
	 * Read lower 4 bytes before upper 4 bytes.
	 * This particular order is required by I/OAT.
	 * If the other order is required, use a pair of
	 * _nvme_mmio_read_4() calls.
	 */
	val = addr32[0];
	val |= (__u64)addr32[1] << 32;

	return val;
#endif
}

static inline void nvme_mmio_write_8(volatile __u64 *addr, __u64 val)
{

#ifdef NVME_MMIO_64BIT
		*addr = val;
#else
	volatile __u32 *addr32 = (volatile __u32 *)addr;

	addr32[0] = (__u32)val;
	addr32[1] = (__u32)(val >> 32);
#endif
}

#endif /* __NVME_ARCH_H__ */
