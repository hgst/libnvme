/*
 * Copyright (c) 2017, Western Digital Corporation or its affiliates.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * Please see COPYING file for license text.
 */

#include "nvme_mem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <numaif.h>

/*
 * Memory management information.
 */
static struct nvme_mem mm;

/*
 * Find where hugetlbfs is mounted.
 */
static int nvme_mem_get_hp_dir(void)
{
	char dev[64], dir[PATH_MAX];
	char type[64], opts[128];
	char *mntdir = NULL;
	int n, tmp1, tmp2;
	char buf[512];
	FILE *f;

	f = fopen("/proc/mounts", "r");
	if (!f) {
		nvme_err("Open /proc/mounts failed\n");
		return -ENOENT;
	}

	while (fgets(buf, sizeof(buf), f)) {
		n = sscanf(buf, "%s %s %s %s %d %d",
			   dev, dir, type, opts, &tmp1, &tmp2);
		if (n != 6)
			continue;

		if (strcmp(type, "hugetlbfs") == 0) {
			mntdir = dir;
			break;
		}
	}

	fclose(f);

	if (!mntdir) {
		nvme_err("hugetlbfs mount not found\n");
		return -ENOENT;
	}

	nvme_debug("hugetlbfs mounted at %s\n", mntdir);

	/* Create a unique subdirectory in the mount point for this process */
	asprintf(&mm.hp_dir, "%s/libnvme.%d.XXXXXX", mntdir, getpid());
	if (!mm.hp_dir)
		return -ENOMEM;
	if (!mkdtemp(mm.hp_dir)) {
		nvme_err("Create hugepage directory %s failed %d (%s)\n",
			 mm.hp_dir, errno, strerror(errno));
		free(mm.hp_dir);
		mm.hp_dir = NULL;
		return -errno;
	}

	nvme_debug("Using hugepage directory %s\n",
		   mm.hp_dir);

	return 0;
}

/*
 * Determine the size of hugepages.
 */
static size_t nvme_mem_get_hp_size(void)
{
	char buf[256];
	size_t size = 0;
	FILE *f;

	f = fopen("/proc/meminfo", "r");
	if (f == NULL) {
		nvme_err("Open /proc/meminfo failed\n");
		return 0;
	}

	while(fgets(buf, sizeof(buf), f)) {
		if (strncmp(buf, "Hugepagesize:", 13) == 0) {
			size = nvme_str2size(&buf[13]);
			break;
		}
	}

	fclose(f);

	nvme_debug("Hugepage size is %zu B\n", size);

	return size;
}

/*
 * Allocate a hugepage descriptor and create its backing file
 * in hugetlbfs.
 */
static struct nvme_hugepage *nvme_mem_alloc_hp(unsigned int node_id)
{
	unsigned long nodemask, maxnodes;
	unsigned int hphash;
	struct nvme_hugepage *hp;
	void *vaddr = MAP_FAILED;
	int ret;

	/* Allocate and initialize a hugepage descriptor */
	hp = calloc(1, sizeof(struct nvme_hugepage));
	if (!hp)
		return NULL;

	hp->size = mm.hp_size;
	hp->size_bits = mm.hp_size_bits;
	hp->node_id = node_id;

	/* Create the hugepage file */
	sprintf(hp->fname, "libnvme.%d-%zu",
		getpid(),
		nvme_atomic64_add_return(&mm.hp_tmp, 1));

	hp->fd = openat(mm.hp_dd, hp->fname,
			O_RDWR | O_LARGEFILE | O_EXCL | O_CREAT,
			S_IRUSR | S_IWUSR);
	if (hp->fd < 0) {
		nvme_err("Open hugepage file %s failed %d (%s)\n",
			 hp->fname, errno, strerror(errno));
		goto err;
	}

	/* Mmap the file */
	vaddr = mmap(NULL, hp->size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE, hp->fd, 0);
	if (vaddr == MAP_FAILED) {
		nvme_err("mmap hugepage file %s failed %d (%s)\n",
			 hp->fname, errno, strerror(errno));
		goto err;
	}

	/*
	 * At this point, there is no page allocated yet. Set the NUMA
	 * memory policy and fault in the page to get a hugepage from
	 * the desired NUMA node.
	 */
	if (node_id == NVME_NODE_ID_ANY) {
		nvme_debug("Allocating hugepage on any node\n");
		nodemask = 0;
		maxnodes = 0;
	} else {
		nvme_debug("Allocating hugepage on node %u\n", node_id);
		nodemask = node_id;
		maxnodes = 1;
	}

	ret = mbind(vaddr, hp->size, MPOL_PREFERRED, &nodemask, maxnodes, 0);
	if (ret != 0) {
		nvme_err("mbind hugepage %p to node %u failed %d (%s)\n",
			 vaddr, node_id,
			 errno, strerror(errno));
		goto err;
	}

	/* Fault in the page */
	memset(vaddr, 0, hp->size);

	/* Lock the page */
	if (mlock(vaddr, hp->size) != 0) {
		nvme_err("Lock hugepage %p failed %d (%s)\n",
			 vaddr, errno, strerror(errno));
		goto err;
	}

	hp->vaddr = (unsigned long) vaddr;
	hp->paddr = nvme_mem_vtophys(vaddr);
	if (hp->paddr == NVME_VTOPHYS_ERROR) {
		nvme_err("Get hugepage %p physical address failed\n",
			 vaddr);
		goto err;
	}

	/* Add the hugepage to the hash table */
	hphash = (hp->vaddr >> mm.hp_size_bits) & NVME_HP_HASH_MASK;
	nvme_spin_lock(&mm.hp_lock);

	LIST_INSERT_HEAD(&mm.hp_list[hphash], hp, link);
	nvme_atomic_inc(&mm.nr_hp);

	nvme_debug("Allocated hugepage %s (%u, hash %u, 0x%lx / 0x%lx)\n",
		   hp->fname, nvme_atomic_read(&mm.nr_hp),
		   hphash, hp->vaddr, hp->paddr);

	nvme_spin_unlock(&mm.hp_lock);

	return hp;

err:
	if (hp->fd >= 0) {
		if (vaddr != MAP_FAILED)
			munmap(vaddr, hp->size);
		close(hp->fd);
		unlinkat(mm.hp_dd, hp->fname, 0);
	}

	free(hp);

	return NULL;
}

/*
 * Free an allocated hugepage.
 * Close and unlink the hugepage backing file and free its descriptor.
 */
static void nvme_mem_free_hp(struct nvme_hugepage *hp)
{

	if (!hp)
		return;

	/* Remove the hugepage from the hash table */
	nvme_spin_lock(&mm.hp_lock);

	nvme_debug("Free hugepage %s (%u, 0x%lx / 0x%lx)\n",
		   hp->fname, nvme_atomic_read(&mm.nr_hp),
		   hp->vaddr, hp->paddr);

	LIST_REMOVE(hp, link);
	nvme_atomic_dec(&mm.nr_hp);

	nvme_spin_unlock(&mm.hp_lock);

	/* Unmap, close and unlink the hugepage file */
	if (munlock((void *)hp->vaddr, hp->size) < 0)
		nvme_crit("Unlock hugepage %s failed %d (%s)\n",
			  hp->fname, errno, strerror(errno));

	if (munmap((void *)hp->vaddr, hp->size) < 0)
		nvme_crit("Unmap hugepage file %s failed %d (%s)\n",
			  hp->fname, errno, strerror(errno));

	if (close(hp->fd) < 0)
		nvme_crit("Close hugepage file %s failed %d (%s)\n",
			  hp->fname, errno, strerror(errno));

	if (unlinkat(mm.hp_dd, hp->fname, 0) < 0)
		nvme_crit("Unlink hugepage file %s failed %d (%s)\n",
			  hp->fname, errno, strerror(errno));

	free(hp);
}

/*
 * Search the hugepage containing the specified address.
 */
struct nvme_hugepage *nvme_mem_search_hp(unsigned long vaddr)
{
	unsigned long hpn = vaddr >> mm.hp_size_bits;
	unsigned long hphash = hpn & NVME_HP_HASH_MASK;
	struct nvme_hugepage *hp, *res = NULL;

	/* Search the hash table */
	nvme_spin_lock(&mm.hp_lock);

	LIST_FOREACH(hp, &mm.hp_list[hphash], link) {
		if ((hp->vaddr >> mm.hp_size_bits) == hpn) {
			res = hp;
			break;
		}
	}

	nvme_spin_unlock(&mm.hp_lock);

	return res;
}

/*
 * Initialize hugepages management.
 */
static int nvme_mem_hp_init(void)
{
	int i, ret;

	/* Initialize hugepages management data */
	nvme_atomic64_init(&mm.hp_tmp);
	nvme_atomic_init(&mm.nr_hp);
	for (i = 0; i < NVME_HP_HASH_SIZE; i++) {
		LIST_INIT(&mm.hp_list[i]);
		nvme_spinlock_init(&mm.hp_lock);
	}

	/* Find out where hugetlbfs is mounted */
	ret = nvme_mem_get_hp_dir();
	if (ret < 0) {
		nvme_crit("hugetlbfs mount point not found\n");
		return ret;
	}

	/* Determine the size of hugepages */
	mm.hp_size = nvme_mem_get_hp_size();
	if (!mm.hp_size) {
		nvme_crit("Failed to determine the size of hugepages\n");
		return -ENOMEM;
	}
	mm.hp_size_bits = nvme_log2(mm.hp_size);

	/* Open hugetlbfs directory */
	ret = open(mm.hp_dir, O_RDONLY | O_DIRECTORY);
	if (ret < 0) {
		nvme_crit("Open hugepage directory %s failed %d (%s)\n",
			  mm.hp_dir, errno, strerror(errno));
		return -errno;
	}

	mm.hp_dd = ret;

	return 0;
}

/*
 * Cleanup hugepages management.
 */
static void nvme_mem_hp_cleanup(void)
{
	struct nvme_hugepage *hp;
	int i;

	/* Free hugepages still in use */
	for (i = 0; i < NVME_HP_HASH_SIZE; i++) {
		while ((hp = LIST_FIRST(&mm.hp_list[i])))
			nvme_mem_free_hp(hp);
	}

	if (mm.hp_dd != -1)
		close(mm.hp_dd);
	rmdir(mm.hp_dir);
	free(mm.hp_dir);
}

/*
 * Add a heap to the specified mempool.
 */
static struct nvme_heap *nvme_mem_pool_grow(struct nvme_mempool *mp)
{
	struct nvme_hugepage *hp;
	struct nvme_heap *heap = NULL;

	/* Allocate a hugepage */
	hp = nvme_mem_alloc_hp(mp->node_id);
	if (!hp)
		return NULL;

	/* Allocate a heap descriptor */
	heap = calloc(1, sizeof(struct nvme_heap));
	if (!heap)
		goto err;

	hp->mp = mp;
	hp->heap = heap;

	heap->hp = hp;
	heap->nr_objs = hp->size >> mp->size_bits;
	heap->nr_free_objs = heap->nr_objs;

	/* Allocate the heap bitmap */
	heap->bitmap = calloc(nvme_align_up(heap->nr_objs, 64) >> 6,
			      sizeof(__u64));
	if (!heap->bitmap)
		goto err;

	/* Add the heap to the memory pool use list */
	LIST_INSERT_HEAD(&mp->use_list, heap, link);
	mp->nr_use++;
	mp->nr_objs += heap->nr_objs;
	mp->nr_free_objs += heap->nr_free_objs;

	nvme_debug("Mempool %zu B: Created heap %p, %zu objects (%zu heaps)\n",
		   mp->size, heap, heap->nr_objs,
		   mp->nr_use + mp->nr_full);

	return heap;

err:
	free(heap);
	nvme_mem_free_hp(hp);

	return NULL;
}

/*
 * Allocate an object from a mempool.
 */
static void *nvme_mem_pool_alloc(struct nvme_mempool *mp, unsigned long *paddr)
{
	struct nvme_heap *heap;
	void *obj = NULL;
	size_t ofst;
	int bit;

	pthread_mutex_lock(&mp->lock);

	/*
	 * Get a heap to allocate from: If there are heaps in use,
	 * keep using them until full. Otherwise, grow the mempool.
	 */
	if (mp->nr_use)
		heap = LIST_FIRST(&mp->use_list);
	else
		heap = nvme_mem_pool_grow(mp);
	if (!heap) {
		nvme_err("No heap for allocation in mempool %zu B\n",
			  mp->size);
		goto out;
	}

	/* Search a free object in the heap */
	bit = find_first_zero_bit(heap->bitmap, heap->nr_objs);
	if (bit < 0) {
		nvme_crit("No free object found in heap size %zu (%zu / %zu)\n",
			  mp->size, heap->nr_free_objs, heap->nr_objs);
		goto out;
	}

	/* Got it: mark object as allocated */
	set_bit(heap->bitmap, bit);
	ofst = (size_t)bit << mp->size_bits;
	obj = (void *) (heap->hp->vaddr + ofst);
	if (paddr)
		*paddr = heap->hp->paddr + ofst;

	mp->nr_free_objs--;
	heap->nr_free_objs--;
	if (nvme_heap_full(heap)) {
		LIST_REMOVE(heap, link);
		mp->nr_use--;
		LIST_INSERT_HEAD(&mp->full_list, heap, link);
		mp->nr_full++;
	}

	nvme_debug("Mempool %zu B: allocated object %p (%p / %d), %zu / %zu objects in use\n",
		   mp->size, obj, heap, bit,
		   mp->nr_objs - mp->nr_free_objs, mp->nr_objs);

out:
	pthread_mutex_unlock(&mp->lock);

	return obj;
}

/*
 * Shrink a mempool.
 */
static void nvme_mem_pool_shrink(struct nvme_mempool *mp,
				 bool force)
{
	struct nvme_heap *heap, *next;
	int n = 0;

	/*
	 * Scan the use list and free unused heaps, but skip
	 * the first empty one.
	 */
	heap = LIST_FIRST(&mp->use_list);
	while (heap) {

		if (!force) {
			if (nvme_heap_empty(heap))
				n++;
			if (!nvme_heap_empty(heap) || n == 1) {
				heap = LIST_NEXT(heap, link);
				continue;
			}
		}

		nvme_debug("Mempool %zu B: Freed heap %p, %zu objects (%zu heaps)\n",
			   mp->size, heap, heap->nr_objs,
			   mp->nr_use + mp->nr_full);

		if (!nvme_heap_empty(heap))
			nvme_warning("Mempool %zu B: Free non-empty heap %p, %zu / %zu objects in use\n",
				     mp->size, heap,
				     mp->nr_objs - mp->nr_free_objs, mp->nr_objs);

		next = LIST_NEXT(heap, link);

		/* Remove the heap from the meory pool empty list */
		LIST_REMOVE(heap, link);
		mp->nr_use--;
		mp->nr_objs -= heap->nr_objs;
		mp->nr_free_objs -= heap->nr_free_objs;

		/* Free resources */
		free(heap->bitmap);
		nvme_mem_free_hp(heap->hp);
		free(heap);

		heap = next;

	}
}

/*
 * Free a mempool object.
 */
static void nvme_mem_pool_free(struct nvme_mempool *mp, struct nvme_heap *heap,
			       void *vaddr)
{
	struct nvme_hugepage *hp = heap->hp;
	unsigned long obj = (unsigned long)vaddr;
	int bit;

	pthread_mutex_lock(&mp->lock);

	if (obj < hp->vaddr || obj >= hp->vaddr + hp->size) {
		nvme_crit("Object %p does not belong to heap 0x%lx + %zu\n",
			   vaddr, hp->vaddr, hp->size);
		goto out;
	}

	bit = (obj - hp->vaddr) >> mp->size_bits;
	if (heap->nr_free_objs == heap->nr_objs ||
	    !test_bit(heap->bitmap, bit)) {
		nvme_crit("Double free on object %p in heap size %zu (%zu / %zu)\n",
			  vaddr, mp->size, heap->nr_free_objs, heap->nr_objs);
		goto out;
	}

	clear_bit(heap->bitmap, bit);

	if (nvme_heap_full(heap)) {
		LIST_REMOVE(heap, link);
		mp->nr_full--;
		LIST_INSERT_HEAD(&mp->use_list, heap, link);
		mp->nr_use++;
	}

	heap->nr_free_objs++;
	mp->nr_free_objs++;

	if (nvme_heap_empty(heap))
		nvme_mem_pool_shrink(mp, false);

	nvme_debug("Mempool %zu B: freed object %p (%p / %d), %zu / %zu objects in use\n",
		   mp->size, (void *)obj, heap, bit,
		   mp->nr_objs - mp->nr_free_objs, mp->nr_objs);

out:
	pthread_mutex_unlock(&mp->lock);
}

/*
 * Allocate memory on the specified NUMA node.
 */
void *nvme_mem_alloc_node(size_t size, size_t align, unsigned int node_id,
			  unsigned long *paddr)
{
	unsigned int size_bits;
	struct nvme_mempool *mp;

	if (size == 0 || (align && !nvme_is_pow2(align))) {
		nvme_err("Invalid allocation request %zu / %zu\n",
			 size, align);
		return NULL;
	}

	if (node_id == NVME_NODE_ID_ANY ||
	    node_id >= nvme_node_max())
		node_id = nvme_node_id();

	nvme_debug("Allocation from CPU %u, NUMA node %u\n",
		   nvme_cpu_id(),
		   nvme_node_id());

	/* Get a suitable memory pool for the allocation */
	size_bits = nvme_log2(nvme_align_pow2(nvme_max(size, align)));
	if (size_bits <= NVME_MP_SIZE_BITS_MIN) {
		mp = &mm.mp[0];
	} else if (size_bits <= NVME_MP_SIZE_BITS_MAX) {
		mp = &mm.mp[size_bits - NVME_MP_SIZE_BITS_MIN];
	} else {
		nvme_debug("No memory pool for %zu B (align %zu B)\n",
			   size, align);
		return NULL;
	}

	nvme_debug("Allocate %zu B, align %zu B => mempool %zu B (order %zu)\n",
		   size, align,
		   mp->size, mp->size_bits);

	return nvme_mem_pool_alloc(mp, paddr);
}

/*
 * Allocate memory on the specified NUMA node.
 */
void *nvme_malloc_node(size_t size, size_t align,
		       unsigned int node_id)
{
	return nvme_mem_alloc_node(size, align, node_id, NULL);
}

/*
 * Free the memory space back to heap.
 */
void nvme_free(void *addr)
{
	struct nvme_hugepage *hp;

	if (!addr)
		return;

	hp = nvme_mem_search_hp((unsigned long)addr);
	if (!hp) {
		nvme_crit("Invalid address %p for free\n", addr);
		return;
	}

	nvme_mem_pool_free(hp->mp, hp->heap, addr);
}

/*
 * Return the physical address of the specified virtual address.
 */
unsigned long nvme_mem_vtophys(void *addr)
{
	unsigned long vaddr = (unsigned long) addr;
	struct nvme_hugepage *hp;
	unsigned long ofst;
	ssize_t ret;
	__u64 ppfn, vpn;

	/* Avoid the system call if this is a hugepage address */
	hp = nvme_mem_search_hp(vaddr);
	if (hp)
		return hp->paddr + vaddr - hp->vaddr;

	/* Read the page frame entry (8B per entry) */
	vpn = (unsigned long)vaddr >> mm.pg_size_bits;
	ofst = (unsigned long)vaddr & mm.pg_size_mask;

	ret = pread(mm.pg_mapfd, &ppfn, 8, vpn << NVME_PFN_SIZE_SHIFT);
	if (ret != NVME_PFN_SIZE) {
		if (ret < 0)
			nvme_err("Read /proc/self/pagemap failed %d (%s)\n",
				 errno, strerror(errno));
		else
			nvme_err("Partial pfn %llu read from /proc/self/pagemap\n",
				 vpn);
		return NVME_VTOPHYS_ERROR;
	}

	return ((ppfn & NVME_PFN_MASK) << mm.pg_size_bits) + ofst;
}

/*
 * Get memory usage statistics for the specified socket.
 */
int nvme_memstat(struct nvme_mem_stats *stats, unsigned int node_id)
{
	struct nvme_mempool *mp;
	unsigned int i;

	if (!stats)
		return -EFAULT;

	if (node_id != NVME_NODE_ID_ANY &&
	    node_id > NVME_NODE_MAX)
		return -EINVAL;

	/* Get stats */
	stats->nr_hugepages = nvme_atomic_read(&mm.nr_hp);
	stats->total_bytes = 0;
	stats->free_bytes = 0;

	for(i = 0; i < NVME_MP_NUM; i++) {
		mp = &mm.mp[i];
		pthread_mutex_lock(&mp->lock);
		stats->total_bytes += mp->nr_objs << mp->size_bits;
		stats->free_bytes += mp->nr_free_objs << mp->size_bits;
		pthread_mutex_unlock(&mp->lock);
	}

        return 0;
}

/*
 * Initialize memory management.
 */
int nvme_mem_init(void)
{
	struct nvme_mempool *mp;
	int size_bits = NVME_MP_SIZE_BITS_MIN;
	int i, ret;

	memset(&mm, 0, sizeof(struct nvme_mem));
	mm.pg_size = sysconf(_SC_PAGESIZE);
	mm.pg_size_bits = nvme_log2(mm.pg_size);
	mm.pg_size_mask = mm.pg_size - 1;

	nvme_debug("System page size: %zu B (order %zu)\n",
		   mm.pg_size,
		   mm.pg_size_bits);

	mm.pg_mapfd = open("/proc/self/pagemap", O_RDONLY);
	if (mm.pg_mapfd < 0) {
		nvme_err("Open /proc/self/pagemap failed %d (%s)\n",
			 errno, strerror(errno));
		return -errno;
	}

	/* Initialize hugepages management */
	ret = nvme_mem_hp_init();
	if (ret != 0)
		return ret;

	/* Initialize memory pools */
	for(i = 0; i < NVME_MP_NUM; i++) {

		mp = &mm.mp[i];

		mp->size_bits = size_bits;
		mp->size = 1UL << size_bits;

		pthread_mutex_init(&mp->lock, NULL);
		LIST_INIT(&mp->use_list);
		LIST_INIT(&mp->full_list);

		size_bits++;

	}

	return 0;
}

/*
 * Cleanup memory resources on exit.
 */
void nvme_mem_cleanup(void)
{
	struct nvme_mempool *mp;
	struct nvme_heap *heap;
	int i;

	/* Cleanup memory pools */
	for(i = 0; i < NVME_MP_NUM; i++) {

		mp = &mm.mp[i];

		while ((heap = LIST_FIRST(&mp->full_list))) {
			LIST_REMOVE(heap, link);
			LIST_INSERT_HEAD(&mp->use_list, heap, link);
			printf("full heap %d\n", i);
		}
		while ((heap = LIST_FIRST(&mp->use_list)))
			nvme_mem_pool_shrink(mp, true);

	}

	/* Cleanup hugepages */
	nvme_mem_hp_cleanup();

	return;
}
