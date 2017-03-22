/*
 * (C) Copyright 2017, Western Digital Corporation.
 * All Rights Reserved.
 *
 * This software is distributed under the terms of the BSD 2-clause license,
 * "as is," without technical support, and WITHOUT ANY WARRANTY, without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. You should have received a copy of the BSD 2-clause license along
 * with libzbc. If not, see  <http://opensource.org/licenses/BSD-2-Clause>.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include "libnvme/nvme.h"

static unsigned long long nvme_info_strsize(unsigned long long val,
					    char *unit)
{
	unsigned long long uval = val;

	if (uval < 1024) {
		strcpy(unit, "");
		return uval;
	}

	uval /= 1024;
	if (uval < 1024) {
		strcpy(unit, "Ki");
		return uval;
	}

	uval /= 1024;
	if (uval < 1024) {
		strcpy(unit, "Mi");
		return uval;
	}

	uval /= 1024;
	strcpy(unit, "Gi");

	return uval;
}

static int nvme_info_ctrlr(struct nvme_ctrlr *ctrlr,
			   struct nvme_ctrlr_stat *cstat)
{
	struct nvme_register_data rdata;
	unsigned long long uval;
	char unit[16];

	/* Get information */
	if (nvme_ctrlr_stat(ctrlr, cstat) != 0) {
		fprintf(stderr, "Get controller info failed\n");
		return -1;
	}

	if (nvme_ctrlr_data(ctrlr, NULL, &rdata) != 0) {
		fprintf(stderr, "Get controller HW data failed\n");
		return -1;
	}

	printf("  Model name: %s\n", cstat->mn);
	printf("  Serial number: %s\n", cstat->sn);
	printf("  HW maximum queue entries: %u\n", rdata.mqes + 1);
	printf("  Maximum queue depth: %u\n", cstat->max_qd);

	uval = nvme_info_strsize(cstat->max_xfer_size, unit);
	printf("  Maximum request size: %llu %sB\n", uval, unit);

	return 0;
}

static int nvme_info_ns(struct nvme_ctrlr *ctrlr,
			struct nvme_ctrlr_stat *cstat)
{
	struct nvme_ns_stat nsstat;
	struct nvme_ns *ns;
	unsigned long long uval;
	char unit[16];
	unsigned int i;

	printf("%u namespaces:\n", cstat->nr_ns);

	for (i = 0; i < cstat->nr_ns; i++) {

		ns = nvme_ns_open(ctrlr, cstat->ns_ids[i]);
		if (!ns) {
			fprintf(stderr, "Open namespace %u failed\n",
				cstat->ns_ids[i]);
			return -1;
		}

		if (nvme_ns_stat(ns, &nsstat) != 0) {
			fprintf(stderr, "Get namespace %u info failed\n",
				cstat->ns_ids[i]);
				nvme_ns_close(ns);
			return -1;
		}

		uval = nvme_info_strsize(nsstat.sector_size * nsstat.sectors,
					 unit);
		printf("  Namespace %u/%u: %lu bytes sectors, %lu sectors (%llu %sB)\n",
		       nsstat.id, cstat->nr_ns,
		       nsstat.sector_size, nsstat.sectors,
		       uval, unit);

		nvme_ns_close(ns);

	}

	return 0;
}

static int nvme_info_qpair(struct nvme_ctrlr *ctrlr,
			   struct nvme_ctrlr_stat *cstat)
{
	struct nvme_qpair_stat *qpstat;
	struct nvme_qpair **qp;
	unsigned int i;
	int ret;

	printf("%u I/O queue pairs:\n", cstat->max_io_qpairs);

	qpstat = calloc(cstat->max_io_qpairs, sizeof(struct nvme_qpair_stat));
	qp = calloc(cstat->max_io_qpairs, sizeof(struct nvme_qpair *));
	if (!qpstat || !qp) {
		fprintf(stderr, "No memory for I/O qpairs info\n");
		return -1;
	}

	for (i = 0; i < cstat->max_io_qpairs; i++) {

		qp[i] = nvme_ioqp_get(ctrlr, 0, 0);
		if (!qp[i]) {
			fprintf(stderr, "Get I/O qpair %d failed\n", i);
			break;
		}

		ret = nvme_qpair_stat(qp[i], &qpstat[i]);
		if (ret) {
			fprintf(stderr,
				"Get I/O qpair %d information failed\n", i);
			break;
		}

		printf("  qpair %u/%u: ID %u, max qd %u, prio %u\n",
		       i + 1, cstat->max_io_qpairs,
		       qpstat[i].id, qpstat[i].qd, qpstat[i].qprio);

	}

	for (i = 0; i < cstat->max_io_qpairs; i++)
		if (qp[i])
			nvme_ioqp_release(qp[i]);

	free(qp);
	free(qpstat);

	return 0;
}

int main(int argc, char **argv)
{
	struct nvme_ctrlr *ctrlr;
	struct nvme_ctrlr_stat cstat;
	int log_level = -1;
	char *dev;
	int i, ret;

	if (argc < 2) {
		printf("Usage: %s [options] <PCI device URL>\n"
		       "Options:\n"
		       "  -v : verbose mode (debug log levelNVME_LOG_NOTICE)\n",
		       argv[0]);
		exit(1);
	}

	for (i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "-v") == 0) {
			log_level = NVME_LOG_DEBUG;
		} else {
			fprintf(stderr,
				"Unknown option \"%s\"\n",
			       argv[i]);
			exit(1);
		}
	}

	dev = argv[i];

	ret = nvme_lib_init(log_level, -1, NULL);
	if (ret != 0) {
		fprintf(stderr,
			"libnvme init failed %d (%s)\n",
			ret, strerror(-ret));
		exit(1);
	}

	printf("Opening NVMe controller %s\n", dev);
	ctrlr = nvme_ctrlr_open(dev, NULL);
	if (!ctrlr) {
		fprintf(stderr, "Open NVMe controller %s failed\n",
			dev);
		return -1;
	}

	ret = nvme_info_ctrlr(ctrlr, &cstat);
	if (ret != 0)
		goto out;

	ret = nvme_info_ns(ctrlr, &cstat);
	if (ret != 0)
		goto out;

	ret = nvme_info_qpair(ctrlr, &cstat);

 out:
	nvme_ctrlr_close(ctrlr);

	return ret;
}
