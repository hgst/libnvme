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

#include "nvme_common.h"
#include "nvme_cpu.h"

#include <dirent.h>

struct nvme_cpu_info cpui;

/*
 * Check if a cpu is present by the presence
 * of the cpu information for it.
 */
static bool nvme_cpu_present(unsigned int cpu_id)
{
	char path[128];

	snprintf(path, sizeof(path),
		 "/sys/devices/system/cpu/cpu%u/topology/core_id",
		 cpu_id);

	return access(path, F_OK) == 0;
}

/*
 * Count the number of sockets.
 */
static unsigned int nvme_socket_count(void)
{
	unsigned int socket;
	char path[128];
	unsigned int n = 0;

	for (socket = 0; socket < NVME_SOCKET_MAX; socket++) {
		snprintf(path, sizeof(path),
			 "/sys/devices/system/node/node%u",
			 socket);
		if (!access(path, F_OK) == 0)
			break;
		n++;
	}

	return n;
}

/*
 * Get the socket ID (NUMA node) of a CPU.
 */
static unsigned int nvme_cpu_socket_id(unsigned int cpu_id)
{
	char path[128];
	unsigned long id;

	snprintf(path, sizeof(path),
		 "/sys/devices/system/cpu/cpu%u/topology/physical_package_id",
		 cpu_id);
	if (nvme_parse_sysfs_value(path, &id) != 0) {
		nvme_err("Parse %s failed\n", path);
		return 0;
	}

	return id;
}

/*
 * Get the core ID of a CPU.
 */
static unsigned int nvme_cpu_core_id(unsigned int cpu_id)
{
	char path[128];
	unsigned long id;

	snprintf(path, sizeof(path),
		 "/sys/devices/system/cpu/cpu%u/topology/core_id",
		 cpu_id);
	if (nvme_parse_sysfs_value(path, &id) != 0) {
		nvme_err("Parse %s failed\n", path);
		return 0;
	}

	return id;
}

/*
 * Get the thread ID of a CPU.
 */
static unsigned int nvme_cpu_thread_id(unsigned int cpu_id)
{
	struct nvme_cpu *cpu = &cpui.cpu[cpu_id];
	unsigned int i, thid = 0;

	for (i = 0; i < cpui.nr_cpus; i++) {
		if (cpui.cpu[i].socket == cpu->socket &&
		    cpui.cpu[i].core == cpu->core)
			thid++;
	}

	return thid;
}

/*
 * Parse /sys/devices/system/cpu to initialize CPU information.
 */
int nvme_cpu_init(void)
{
	struct nvme_cpu *cpu;
	unsigned int i;

	memset(&cpui, 0, sizeof(struct nvme_cpu_info));
	cpui.nr_sockets = nvme_socket_count();

	for (i = 0; i < NVME_CPU_MAX; i++) {

		cpu = &cpui.cpu[i];
		cpu->id = -1;

		/* init cpuset for per lcore config */
		cpu->present = nvme_cpu_present(i);
		if (!cpu->present) {
			continue;
		}

		cpu->id = i;
		cpu->socket = nvme_cpu_socket_id(i);
		cpu->core = nvme_cpu_core_id(i);
		cpu->thread = nvme_cpu_thread_id(i);

		cpui.nr_cpus++;
		if (cpu->thread == 0)
			cpui.nr_cores++;

		nvme_debug("CPU %02u: socket %02u, core %02u, thread %u\n",
			   cpu->id, cpu->socket, cpu->core, cpu->thread);

	}

	nvme_info("Detected %u CPUs: %u sockets, %u cores, %u threads\n",
		  cpui.nr_cpus,
		  cpui.nr_sockets,
		  cpui.nr_cores,
		  cpui.nr_cpus);

	return 0;
}

/*
 * Get caller current CPU.
 */
struct nvme_cpu *nvme_get_cpu(void)
{
	int cpu;

	/*
	 * Get current CPU. If trhe caller thread is not pinned down
	 * to a particular CPU using sched_setaffinity, this result
	 * may be only temporary.
	 */
	cpu = sched_getcpu();
	if (cpu < 0) {
		nvme_err("sched_getcpu failed %d (%s)\n",
			 errno, strerror(errno));
		return NULL;
	}

	if (cpu >= (int)cpui.nr_cpus) {
		nvme_err("Invalid CPU number %d (Max %u)\n",
			 cpu, cpui.nr_cpus - 1);
		return NULL;
	}

	return &cpui.cpu[cpu];
}


