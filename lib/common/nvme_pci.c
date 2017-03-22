/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
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
#include "nvme_pci.h"

/*
 * Initialize PCI subsystem.
 */
int nvme_pci_init(void)
{
	int ret;

	ret = pci_system_init();
	if (ret) {
		fprintf(stderr, "pci_system_init failed %d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * Check if a device has a driver binded.
 */
static int nvme_pci_device_has_kernel_driver(struct pci_device *dev)
{
	char linkname[NVME_PCI_PATH_MAX];
	char driver[NVME_PCI_PATH_MAX];
	char *driver_name;
	ssize_t driver_len;

	snprintf(linkname, sizeof(linkname),
		 "/sys/bus/pci/devices/%04x:%02x:%02x.%1u/driver",
		 dev->domain, dev->bus, dev->dev, dev->func);

	memset(driver, 0, sizeof(driver));
	driver_len = readlink(linkname, driver, sizeof(driver));
	if ((driver_len <= 0) || (driver_len >= NVME_PCI_PATH_MAX))
		return 0;

	driver_name = basename(driver);

	nvme_info("Kernel driver %s attached to NVME controller %04x:%02x:%02x.%1u\n",
		  driver_name,
		  dev->domain, dev->bus, dev->dev, dev->func);

	/* Those are OK, we can ignore */
	if (strcmp(driver_name, "uio_pci_generic") == 0 ||
	    strcmp(driver_name, "vfio-pci") == 0)
		return 0;

	nvme_err("Device in use\n");

	return 1;
}

/*
 * Search a PCI device and grab it if found.
 */
struct pci_device *nvme_pci_device_probe(const struct pci_slot_match *slot)
{
	struct pci_device_iterator *pci_dev_iter;
	struct pci_device *pci_dev = NULL;
	int ret = -ENODEV;

	pci_dev_iter = pci_slot_match_iterator_create(slot);
	pci_dev = pci_device_next(pci_dev_iter);
	if (pci_dev)
		ret = pci_device_probe(pci_dev);
	pci_iterator_destroy(pci_dev_iter);

	if (ret != 0)
		return NULL;

	if (pci_dev->device_class != NVME_PCI_CLASS) {
		nvme_err("Device PCI class is not NVME\n");
		pci_dev = NULL;
	}

	if (nvme_pci_device_has_kernel_driver(pci_dev))
		return NULL;

	return pci_dev;
}

/*
 * Get a device serial number.
 */
int nvme_pci_device_get_serial_number(struct pci_device *dev,
				      char *sn, size_t len)
{
	uint32_t pos, header = 0;
	uint32_t i, buf[2];
	int ret;

	if (len < 17)
		return -1;

	ret = nvme_pcicfg_read32(dev, &header, NVME_PCI_CFG_SIZE);
	if (ret || !header)
		return -1;

	pos = NVME_PCI_CFG_SIZE;
	while (1) {

		if ((header & 0x0000ffff) == NVME_PCI_EXT_CAP_ID_SN &&
		    pos != 0) {
			for (i = 0; i < 2; i++) {
				/* skip the header */
				pos += 4;
				ret = nvme_pcicfg_read32(dev, &buf[i],
								 pos);
				if (ret)
					return -1;
			}
			sprintf(sn, "%08x%08x", buf[1], buf[0]);
			return 0;
		}

		pos = (header >> 20) & 0xffc;

		/* 0 if no other items exist */
		if (pos < NVME_PCI_CFG_SIZE)
			return -1;

		ret = nvme_pcicfg_read32(dev, &header, pos);
		if (ret)
			return -1;

	}

	return -1;
}

/*
 * Reset a PCI device.
 */
int nvme_pci_device_reset(struct pci_device *dev)
{
	char filename[NVME_PCI_PATH_MAX];
	char *buf = "1";
	FILE *fd;
	int ret;

	snprintf(filename, sizeof(filename),
		 "/sys/bus/pci/devices/%04x:%02x:%02x.%1u/reset",
		 dev->domain, dev->bus, dev->dev, dev->func);

	nvme_debug("Resetting PCI device (%s)\n",
		   filename);

	fd = fopen(filename, "w");
	if (!fd)
		return -1;

	if (fwrite(buf, strlen(buf), 1, fd) != strlen(buf))
		ret = -1;
	else
		ret = 0;

	fclose(fd);

	return ret;
}

