/*
 * Copyright (c) 2017, Western Digital Corporation or its affiliates.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Please see COPYING file for license text.
 */

#ifndef __NVME_MEMORY_H_
#define __NVME_MEMORY_H_

#include "nvme_common.h"
#include "nvme_spinlock.h"
#include "nvme_atomic.h"
#include "nvme_cpu.h"

#include <pthread.h>
#include <sys/user.h>

/*
 * Physical address.
 */
typedef __u64 phys_addr_t;

#define NVME_VTOPHYS_ERROR	(~0ULL)

/*
 * Page frame numbers are stored on 8 bytes in /proc/self/pagemap.
 */
#define NVME_PFN_SIZE_SHIFT	3
#define NVME_PFN_SIZE		(1 << NVME_PFN_SIZE_SHIFT)

/*
 * Page frame numbers are bits 0-54
 * (see pagemap.txt in linux Documentation).
 */
#define NVME_PFN_MASK		0x7fffffffffffffULL

/*
 * Maximum number of NUMA nodes.
 */
#define NVME_NODE_MAX		NVME_SOCKET_MAX

/*
 * Huge-page descriptor.
 */
struct nvme_hugepage {

	/*
	 * For listing internally.
	 */
	LIST_ENTRY(nvme_hugepage)	link;

	/*
	 * This hugepage size in Bytes.
	 */
	size_t				size;
	size_t				size_bits;

	/*
	 * Virtual and physical address of the page.
	 */
	unsigned long			vaddr;
	unsigned long			paddr;

	/*
	 * The NUMA node this page belongs to.
	 */
	unsigned int			node_id;

        /*
	 * The page file path and descriptor.
	 */
	int 				fd;
	char 				fname[128];

	/*
	 * The memory pool owing this hugepage.
	 */
	struct nvme_mempool		*mp;

	/*
	 * The heap using this hugepage.
	 */
	struct nvme_heap		*heap;

};

/*
 * Per hugepage heap descriptor.
 */
struct nvme_heap {

	/*
	 * For listing in a memory pool.
	 */
	LIST_ENTRY(nvme_heap)		link;

	/*
	 * The backing hugepage used.
	 */
	struct nvme_hugepage		*hp;

	/*
	 * Total number of objects.
	 */
	size_t				nr_objs;

	/*
	 * Number of free objects.
	 */
	size_t				nr_free_objs;

	/*
	 * Slot allocation state bitmap (0 = free, 1 = allocated).
	 */
	__u8				*bitmap;

};

/*
 * Test if a heap is empty or full.
 */
#define nvme_heap_empty(heap)	((heap)->nr_free_objs == (heap)->nr_objs)
#define nvme_heap_full(heap)	((heap)->nr_free_objs == 0)

/*
 * Memory pool descriptor.
 * A memory pool is a set of heaps, each heap being
 * built on top of a single hugpage. All heaps of the
 * memory pool have the same slot size.
 */
struct nvme_mempool {

	/*
	 * Mempool lock.
	 */
	pthread_mutex_t			lock;

	/*
	 * Objects size.
	 */
	size_t				size_bits;
	size_t				size;

	/*
	 * Total number of objects.
	 */
	size_t				nr_objs;

	/*
	 * Total number of free objects.
	 */
	size_t				nr_free_objs;

	/*
	 * The NUMA node this memory pool belongs to.
	 */
	unsigned int			node_id;

	/*
	 * List of heaps in use but not full.
	 */
	size_t				nr_use;
	LIST_HEAD(, nvme_heap)		use_list;

	/*
	 * List of full heaps.
	 */
	size_t				nr_full;
	LIST_HEAD(, nvme_heap)		full_list;

};

/*
 * Static mempool sizes.
 */
enum nvme_mempool_size {

	/* 128 Bytes */
	NVME_MP_SIZE_BITS_MIN	= 7,

	/* 2 MBytes */
	NVME_MP_SIZE_BITS_MAX	= 21,

	/* All powers of 2 in between */
	NVME_MP_NUM      	= 15

};

/*
 * Hugepage hash table size.
 */
#define NVME_HP_HASH_SIZE	32
#define NVME_HP_HASH_MASK	(NVME_HP_HASH_SIZE - 1)

/*
 * Memory managament data.
 */
struct nvme_mem {

	/*
	 * System memory page size.
	 */
	size_t				pg_size;
	size_t				pg_size_bits;
	size_t				pg_size_mask;

	/*
	 * /proc/self/pagemap file descriptor.
	 */
	int				pg_mapfd;

	/*
	 * Directory where to store hugepage files
	 * (within hugetlbfs mount).
	 */
	char 				*hp_dir;
	int				hp_dd;

	/*
	 * Huge page size.
	 */
	size_t				hp_size;
	size_t				hp_size_bits;

	/*
	 * For generating huge-page file names.
	 */
	nvme_atomic64_t			hp_tmp;

	/*
	 * Hugepage management spinlock.
	 */
	nvme_spinlock_t			hp_lock;

	/*
	 * Number of hugepages currently allocated.
	*/
	nvme_atomic_t			nr_hp;

	/*
	 * Hugepage hash table (array of lists).
	 */
	LIST_HEAD(, nvme_hugepage)	hp_list[NVME_HP_HASH_SIZE];

	/*
	 * Static memory pools.
	 */
	struct nvme_mempool		mp[NVME_MP_NUM];

};

/*
 * Initialize memory management.
 */
extern int nvme_mem_init(void);

/*
 * Cleanup memory resources on exit.
 */
extern void nvme_mem_cleanup(void);

/*
 * Allocate memory on the specified NUMA node.
 */
extern void *nvme_mem_alloc_node(size_t size, size_t align,
				 unsigned int node_id, unsigned long *paddr);

/*
 * Return the physical address of the specifed virtual address.
 */
extern unsigned long nvme_mem_vtophys(void *vaddr);

/*
 * Maximum number of NUMA nodes.
 */
#define nvme_node_max()		(cpui.nr_sockets)

/*
 * Current NUMA node.
 */
#define nvme_node_id()		nvme_socket_id()

#endif /* __NVME_MEMORY_H_ */
