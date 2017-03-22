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

#ifndef __NVME_CPU_H__
#define __NVME_CPU_H__

#include "nvme_common.h"

#include <pthread.h>

/*
 * Maximum number of CPU supported.
 */
#define NVME_CPU_MAX	        64

/*
 * Undefined CPU ID.
 */
#define NVME_CPU_ID_ANY	        UINT_MAX

/*
 * Maximum number of sockets supported.
 */
#define NVME_SOCKET_MAX	        32

/*
 * Undefined SOCKET ID.
 */
#define NVME_SOCKET_ID_ANY	UINT_MAX

/*
 * System CPU descriptor.
 */
struct nvme_cpu {

	/*
	 * CPU ID.
	 */
	unsigned int		id;

	/*
	 * Socket number.
	 */
	unsigned int		socket;

	/*
	 * Core number.
	 */
	unsigned int		core;

	/*
	 * Thread number.
	 */
	unsigned int		thread;

	/*
	 * CPU preset.
	 */
	bool			present;

};

/*
 * System CPU information.
 */
struct nvme_cpu_info {

	/*
	 * Total number of CPUs.
	 */
	unsigned int		nr_cpus;

	/*
	 * CPU information.
	 */
	struct nvme_cpu		cpu[NVME_CPU_MAX];

	/*
	 * Number of sockets.
	 */
	unsigned int		nr_sockets;

	/*
	 * Number of CPU cores.
	 */
	unsigned int		nr_cores;


};

extern struct nvme_cpu_info cpui;

/*
 * Initialize system CPU information.
 */
extern int nvme_cpu_init(void);

/*
 * Return the CPU of the caller.
 */
extern struct nvme_cpu *nvme_get_cpu(void);

/*
 * Return the CPU ID of the caller.
 */
static inline unsigned int nvme_cpu_id(void)
{
	struct nvme_cpu *cpu = nvme_get_cpu();

	return cpu ? cpu->id : NVME_CPU_ID_ANY;
}

/*
 * Return the Socket ID of the caller.
 */
static inline unsigned int nvme_socket_id(void)
{
	struct nvme_cpu *cpu = nvme_get_cpu();

	return cpu ? cpu->socket : NVME_SOCKET_ID_ANY;
}

#endif /* __NVME_CPU_H__ */
