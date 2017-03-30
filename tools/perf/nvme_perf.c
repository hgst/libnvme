/*
 * (C) Copyright 2017, Western Digital Corporation.
 * All Rights Reserved.
 *
 * This software is distributed under the terms of the BSD 2-clause license,
 * "as is," without technical support, and WITHOUT ANY WARRANTY, without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. You should have received a copy of the BSD 2-clause license along
 * with libnvme. If not, see  <http://opensource.org/licenses/BSD-2-Clause>.
 */

#define _GNU_SOURCE

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <pciaccess.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "nvme_perf.h"

static nvme_perf_t nt;

/*
 * Get current time in nano seconds.
 */
static inline unsigned long long nvme_perf_time_nsec(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	return (unsigned long long) ts.tv_sec * 1000000000LL
		+ (unsigned long long) ts.tv_nsec;
}

/*
 * Elapsed test time in seconds.
 */
static inline int nvme_perf_elapsed_secs(void)
{
	return (nvme_perf_time_nsec() - nt.start) / 1000000000;
}

static void nvme_perf_usage(char *cmd)
{

	printf("Usage: %s [options] <path> <io size (B)>\n"
	       "Options:\n"
	       "  -h | --help : Print this message\n"
	       "  -l <level>  : Specify a log level between 0 and 8\n"
	       "                0 = none (disable all messages)\n"
	       "                1 = emergency (system is unusable)\n"
	       "                2 = alert (action must be taken immediately)\n"
	       "                3 = critical (critical conditions)\n"
	       "                4 = error (error conditions)\n"
	       "                5 = warning (warning conditions)\n"
	       "                6 = notice (normal but significant condition) (default)\n"
	       "                7 = info (informational messages)\n"
	       "                8 = debug (debug-level messages */\n"
	       "  -t <secs>   : Set the run time (default: 10 seconds)\n"
	       "  -cpu <id>   : Run on the specified CPU (default: 0)\n"
	       "  -ns <id>    : Access the specified namespace (default: 1)\n"
	       "  -rw <perc>  : <perc> %% reads and (100 - <perc>) %% writes\n"
	       "  -qd <num>   : Issue I/Os with queue depth of <num>\n"
	       "                Default is 1, maximum depends on the device\n"
	       "  -rnd        : Do random I/Os (default: sequential)\n",
	       cmd);

	exit(1);
}

static void nvme_perf_get_params(int argc, char **argv)
{
	int i;

	if (argc < 3)
		nvme_perf_usage(argv[0]);

	/* Initialize defaults */
	memset(&nt, 0, sizeof(nvme_perf_t));
	nt.log_level = -1;
	nt.cpu = 0;
	nt.run_secs = 10;
	nt.ns_id = 1;
	nt.qd = 1;
	nt.rw = 100;
	srand(getpid());

	/* Parse options */
	for (i = 1; i < argc - 1; i++) {

		if (strcmp(argv[i], "-h") == 0 ||
		    strcmp(argv[i], "--help") == 0) {

			nvme_perf_usage(argv[0]);

		} else if (strcmp(argv[i], "-l") == 0) {

			i++;
			if (i == (argc - 1))
				nvme_perf_usage(argv[0]);

			nt.log_level = atoi(argv[i]);

		} else if (strcmp(argv[i], "-t") == 0) {

			i++;
			if (i == (argc - 1))
				nvme_perf_usage(argv[0]);

			nt.run_secs = atoi(argv[i]);
			if (nt.run_secs <= 0) {
				fprintf(stderr,
					"Invalid run time %s\n",
					argv[i]);
				exit(1);
			}

		} else if (strcmp(argv[i], "-cpu") == 0) {

			i++;
			if (i == (argc - 1))
				nvme_perf_usage(argv[0]);

			nt.cpu = atoi(argv[i]);
			if (nt.cpu < 0) {
				fprintf(stderr,
					"Invalid CPU number %s\n",
					argv[i]);
				exit(1);
			}

		} else if (strcmp(argv[i], "-ns") == 0) {

			i++;
			if (i == (argc - 1))
				nvme_perf_usage(argv[0]);

			nt.ns_id = atoi(argv[i]);
			if (nt.ns_id <= 0) {
				fprintf(stderr,
					"Invalid namespace ID %s\n",
					argv[i]);
				exit(1);
			}

		} else if (strcmp(argv[i], "-rw") == 0) {

			i++;
			if (i == (argc - 1))
				nvme_perf_usage(argv[0]);

			nt.rw = atoi(argv[i]);
			if ((nt.rw < 0) || (nt.rw > 100)) {
				fprintf(stderr,
					"Invalid read percentage %s\n",
					argv[i]);
				exit(1);
			}

		} else if (strcmp(argv[i], "-qd") == 0) {

			i++;
			if (i == (argc - 1))
				goto err;

			nt.qd = atoi(argv[i]);
			if (nt.qd <= 0) {
				fprintf(stderr,
					"Invalid queue depth %s\n",
					argv[i]);
				exit(1);
			}

		} else if (strcmp(argv[i], "-rnd") == 0) {

			nt.rnd = 1;

		} else if (argv[i][0] == '-') {

			fprintf(stderr,
				"Unknown option %s\n",
				argv[i]);
			exit(1);

		} else {

			break;

		}

	}

	/* There should be 2 arguments left (<path> <io size (B)>) */
	if ((argc - 1) - i != 1)
		nvme_perf_usage(argv[0]);

	/* Get path */
	nt.path = argv[argc - 2];

	/* Get I/O size */
	nt.io_size = atoi(argv[argc - 1]);
	if (nt.io_size <= 0) {
		fprintf(stderr,
			"Invalid I/O size %s\n",
			argv[argc - 1]);
		exit(1);
	}

	return;
err:

	fprintf(stderr, "Invalid command line\n");
	nvme_perf_usage(argv[0]);
}

static int nvme_perf_open_device(struct nvme_ctrlr_opts *opts)
{
	struct nvme_ctrlr_stat cstat;
	struct nvme_ns_stat nsstat;

	/* Probe the NVMe controller */
	printf("Opening NVMe controller %s\n",
	       nt.path);

	nt.ctrlr = nvme_ctrlr_open(nt.path, opts);
	if (!nt.ctrlr) {
		fprintf(stderr, "Open NVMe controller %s failed\n",
			nt.path);
		return -1;
	}

	/* Get information */
	if (nvme_ctrlr_stat(nt.ctrlr, &cstat) != 0) {
		fprintf(stderr, "Get NVMe controller %s info failed\n",
			nt.path);
		return -1;;
	}

	nt.slot.domain = cstat.domain;
	nt.slot.bus = cstat.bus;
	nt.slot.dev = cstat.dev;
	nt.slot.func = cstat.func;
	nt.nr_ns = cstat.nr_ns;
	nt.max_qd = cstat.max_qd;

	if (cstat.io_qpairs != opts->io_queues)
		printf("Number of IO qpairs limited to %u\n",
		       cstat.io_qpairs);

	sprintf(nt.ctrlr_name, "%s (%s)", cstat.mn,
		cstat.sn);

	printf("Attached NVMe controller %s (%u namespace%s)\n",
	       nt.ctrlr_name, nt.nr_ns, (nt.nr_ns> 1) ? "s" : "");

	/* Open the name space */
	nt.ns = nvme_ns_open(nt.ctrlr, nt.ns_id);
	if (!nt.ns) {
		printf("Open NVMe controller %04x:%02x:%02x.%1u "
		       "name space %u failed\n",
		       nt.slot.domain,
		       nt.slot.bus,
		       nt.slot.dev,
		       nt.slot.func,
		       nt.ns_id);
		return -1;
	}

	if (nvme_ns_stat(nt.ns, &nsstat) != 0) {
		fprintf(stderr, "Get name space %u info failed\n",
			nt.ns_id);
		return -1;
	}

	nt.sectsize = nsstat.sector_size;
	nt.nr_sectors = nsstat.sectors;

	return 0;
}

static void
nvme_perf_sigcatcher(int sig)
{
	nt.abort = sig;
}

static int
nvme_perf_init(void)
{
	nvme_perf_io_t *io;
	cpu_set_t cpu_mask;
	struct nvme_ctrlr_opts opts;
	struct nvme_qpair_stat qpstat;
	int i, ret;

	/* Setup signal handler */
	signal(SIGQUIT, nvme_perf_sigcatcher);
	signal(SIGINT, nvme_perf_sigcatcher);
	signal(SIGTERM, nvme_perf_sigcatcher);

	/* Pin down the process on the target CPU */
	CPU_ZERO(&cpu_mask);
        CPU_SET(nt.cpu, &cpu_mask);
        ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t),
				     &cpu_mask);
        if (ret ) {
		fprintf(stderr,
			"pthread_setaffinity_np failed %d (%s)\n",
			ret,
			strerror(ret));
		return -1;
        }
        sched_yield();

	/* Initialize libnvme */
	ret = nvme_lib_init(nt.log_level, -1, NULL);
	if (ret != 0) {
		fprintf(stderr,
			"libnvme init failed %d (%s)\n",
			ret, strerror(-ret));
		exit(1);
	}

	/* Initialize the controller options */
	memset(&opts, 0, sizeof(struct nvme_ctrlr_opts));
	opts.io_queues = 1;

	/* Grab the device */
	ret = nvme_perf_open_device(&opts);
	if (ret)
		return -1;

	if (nt.io_size % nt.sectsize) {
		fprintf(stderr,
			"Invalid I/O size %zu B: must be a multiple "
			"of the sector size %zu B\n",
			nt.io_size,
			nt.sectsize);
		return -1;
	}

	if (nt.max_qd < (unsigned int)nt.qd) {
		fprintf(stderr,
			"Queue depth has to be less than the maximum queue "
			"entries authorized (%u)\n",
			nt.max_qd);
		return -1;
	}

	/* Get an I/O queue pair */
	nt.qpair = nvme_ioqp_get(nt.ctrlr, 0, 0);
	if (!nt.qpair) {
		fprintf(stderr, "Allocate I/O qpair failed\n");
		return -1;
	}

	ret = nvme_qpair_stat(nt.qpair, &qpstat);
	if (ret) {
		fprintf(stderr, "Get I/O qpair information failed\n");
		return -1;
	}
	printf("Qpair %u, depth: %u\n", qpstat.id, qpstat.qd);

	/* Allocate I/Os */
	nt.io = calloc(nt.qd, sizeof(nvme_perf_io_t));
	if (!nt.io) {
		fprintf(stderr, "Allocate I/O array failed\n");
		return -1;
	}

	/* Allocate I/O buffers */
	for (i = 0; i < nt.qd; i++) {
		io = &nt.io[i];
		io->size = nt.io_size / nt.sectsize;
		io->buf = nvme_zmalloc(nt.io_size, nt.sectsize);
		if (!io->buf) {
			fprintf(stderr, "io buffer allocation failed\n");
			return -1;
		}
		nvme_perf_ioq_add(&nt.free_ioq, io);
	}

	return 0;
}

static void
nvme_perf_end(void)
{
	nvme_perf_io_t *io;
	int i;

	/* Close device file */
	if (nt.ctrlr) {

		printf("Detaching NVMe controller %04x:%02x:%02x.%x\n",
		       nt.slot.domain,
		       nt.slot.bus,
		       nt.slot.dev,
		       nt.slot.func);

		if (nt.qpair)
			nvme_ioqp_release(nt.qpair);

		if (nt.ns)
			nvme_ns_close(nt.ns);

		nvme_ctrlr_close(nt.ctrlr);

	}

	if (nt.io) {
		for (i = 0; i < nt.qd; i++) {
			io = &nt.io[i];
			if (io->buf)
				nvme_free(io->buf);
		}
		free(nt.io);
	}

	return;
}

static void
nvme_perf_io_end(void *arg,
		 const struct nvme_cpl *cpl)
{
	nvme_perf_io_t *io = arg;

	nvme_perf_ioq_remove(&nt.pend_ioq, io);
	nvme_perf_ioq_add(&nt.free_ioq, io);

	nt.io_count++;
	nt.io_bytes += nt.io_size;
}

static int
nvme_perf_set_io(nvme_perf_io_t *io)
{
	unsigned long long ofst;
	int rw;

	/* Decide on read or write based on read/write ratio */
	if ( nt.rw == 100 ) {
		rw = NVME_TEST_READ;
	} else if ( nt.rw == 0 ) {
		rw = NVME_TEST_WRITE;
	} else {
		rw = (int)((100UL * (unsigned long)rand())
			   / (unsigned long)RAND_MAX);
		if (rw <= nt.rw)
			rw = NVME_TEST_READ;
		else
			rw = NVME_TEST_WRITE;
	}

	/* Setup I/O offset */
	if (nt.rnd)
		/* Random I/O offset */
		ofst = (double)(nt.nr_sectors - io->size) * (double) rand()
			/ (double) RAND_MAX;
	else {
		ofst = nt.io_ofst / nt.sectsize;
		nt.io_ofst += nt.io_size;
		if (nt.io_ofst >= nt.nr_sectors * nt.sectsize)
			nt.io_ofst = 0;
	}
	io->ofst = ofst;

	return rw;
}

static int
nvme_perf_submit_io(void)
{
	nvme_perf_io_t *io;
	ssize_t ret;
	int rw;

	/* Prepare I/Os */
	while ((io = (nvme_perf_io_t *) nvme_perf_ioq_get(&nt.free_ioq)) &&
	       !nt.abort) {

		nvme_perf_ioq_add(&nt.pend_ioq, io);

		rw = nvme_perf_set_io(io);
		if (rw == NVME_TEST_READ)
			ret = nvme_ns_read(nt.ns, nt.qpair,
					   io->buf,
					   io->ofst,
					   io->size,
					   nvme_perf_io_end, io, 0);
		else
			ret = nvme_ns_write(nt.ns, nt.qpair,
					    io->buf,
					    io->ofst,
					    io->size,
					    nvme_perf_io_end, io, 0);

		if (ret) {
			fprintf(stderr, "Submit I/O failed\n");
			nvme_perf_ioq_remove(&nt.pend_ioq, io);
			nvme_perf_ioq_add(&nt.free_ioq, io);
			nt.abort = 1;
			return -1;
		}

	}

	return 0;
}

/**
 * Run the test: do I/Os.
 */
static void
nvme_perf_run(void)
{

	/* Start */
	nt.start = nvme_perf_time_nsec();

	/* Run for requested time */
	while(nvme_perf_elapsed_secs() < nt.run_secs &&
	      !nt.abort) {

		if (nvme_perf_submit_io() != 0)
			break;

		while (nvme_perf_ioq_empty(&nt.free_ioq))
			nvme_ioqp_poll(nt.qpair, nt.qd);

	}

	/* Wait for remaining started I/Os */
	while (!nvme_perf_ioq_empty(&nt.pend_ioq))
		nvme_ioqp_poll(nt.qpair, nt.qd);

	/* Stop */
	nt.end = nvme_perf_time_nsec();
}

int main(int argc, char **argv)
{
	unsigned long long elapsed;
	long long rate;
	double sz;
	char *unit;
	int ret;

	/* Parse command line */
	nvme_perf_get_params(argc, argv);

	/* Initialize */
	ret = nvme_perf_init();
	if (ret)
		goto out;

	sz = (double)(nt.nr_sectors * nt.sectsize) / (1024 * 1024 * 1024);
	if (sz > 1) {
		unit = "Gi";
	} else {
		unit = "Mi";
		sz = (double)(nt.nr_sectors * nt.sectsize) / (1024 *1024);
	}

	printf("Device %04x:%02x:%02x.%x, namespace %d:\n"
	       "    %.03F %sB capacity (%llu sectors of %zu B)\n",
	       nt.slot.domain, nt.slot.bus, nt.slot.dev, nt.slot.func,
	       nt.ns_id,
	       sz, unit,
	       nt.nr_sectors,
	       nt.sectsize);

	printf("Starting test on CPU %d for %d seconds:\n"
	       "    %d %% read I/O, %d %% write I/Os\n"
	       "    %zu B I/O size, %s access, qd %d\n",
	       nt.cpu, nt.run_secs,
	       nt.rw, 100 - nt.rw,
	       nt.io_size,
	       nt.rnd ? "random" : "sequential",
	       nt.qd);

	/* Run test */
	nvme_perf_run();

	elapsed = nt.end - nt.start;
	if (elapsed && nt.io_count) {

		rate = nt.io_bytes * 1000000000 / elapsed;

		printf("-> %lld I/Os in %.03F secs\n"
		       "    %.03F MB/sec, %lld IOPS\n"
		       "    %.03F usecs average I/O latency\n",
		       nt.io_count,
		       (double)elapsed / 1000000000.0,
		       (double)rate / 1000000.0,
		       nt.io_count * 1000000000 / elapsed,
		       ((double)elapsed / nt.io_count) / 1000.0);

	}

out:
	nvme_perf_end();

	return ret;
}
