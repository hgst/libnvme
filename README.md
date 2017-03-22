
LIBNVME
=======

libnvme provides a user space driver for NVMe PCI devices.

libnvme is based on code  from the Storage Performance Development Kit
(SPDK)  available  at  https://github.com/spdk/spdk.  in  its  current
version, libnvme API is very similar to that of SPDK. However, libnvme
removes all dependencies on the  Data Plane Development Kit (DPDK). To
this  end, some  functions  necessary to  the  library operation  were
reimplemented.  This mainly  consists of  memory management  functions
using  huge-pages  mappings in  order  to  gain access  to  physically
contiguous large memory areas.

In addition  to the device  driver library itself,  a set of  tools is
also  provided  for quick  testing  and  as  examples of  the  library
functions use.

License
=======

libnvme is  distributed under  the terms  of the  of the  BSD 2-clause
license ("Simplified  BSD License"  or "FreeBSD  License"). A  copy of
this  license  with  the  library   copyright  can  be  found  in  the
COPYING.BSD file.

Since  libnvme  is  based  on  SPDK code,  libnvme  also  retains  the
copyright of SPDK as indicated in the COPYING file.

libnvme  and all  its example  applications are  distributed "as  is,"
without technical support, and WITHOUT  ANY WARRANTY, without even the
implied  warranty  of  MERCHANTABILITY  or FITNESS  FOR  A  PARTICULAR
PURPOSE.  Along with  libnvme, you should have received a  copy of the
BSD      2-clause      license.       If     not,      please      see
<http://opensource.org/licenses/BSD-2-Clause>.

Documentation
=============

libnvme API documentation  can be generated using doxygen. To generate
documentation  in  html  format,   the  libnvme.doxygen  file  in  the
documentation directory can be used.

    > cd documentation
    > doxygen libnvme.doxygen

Contact and Bug Reports
=======================

Please contact Damien Le Moal (damien.lemoal@wdc.com) or
 Christophe Louargant (christophe.louargant@wdc.com) to report problems.

Requirements
============

To build libnvme, the following dependencies must be installed.

* libpciaccess  and associated  development headers  (libpciaccess and
  libpciaccess-devel)
* libnuma  and  associated  development  headers (numactl-libs and
  numactl-devel)
* autotools GNU  development tools (autoconf and automake)

Compilation
===========

To compile and install libnvme, execute the following commands.

    > sh ./autogen.sh
    > ./configure
    > make
    > sudo make install

The library  files are installed  by default in /usr/lib.  The library
header files are installed  in /usr/include/libnvme. The example tools
provided are installed in /usr/bin.

Execution Prerequisites
=======================

libnvme requires some hugepages to be available for allocation through
the hugetlbfs file system. This implies first that some hugepages must
be reserved. The following command, executed as root, achieves that.

    echo 32 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

Next,  a mount  point for  hugetlbfs must  exists. For  a system  with
transparent hugepage  support enabled, this  is generally the  case by
default. This can be checked using the following command.

    > cat /proc/mounts | grep hugetlbfs
    hugetlbfs /dev/hugepages hugetlbfs rw,seclabel,relatime 0 0

If no hugetlbfs  mount exists, one can be created  using the following
commands.

    > mkdir -p /mnt/hugepages
    > mount -t hugetlbfs nodev /mnt/hugepages

libnvme, upon  initialization, will  automatically detect and  use the
first available hugetlbfs mount.

Target  PCI  NVMe devices  must  be  unbound  from the  native  kernel
drivers.  This can  be done  manually per  device using  the following
command.

    > echo "0000:01:00.0" > /sys/bus/pci/drivers/nvme/unbind

Where "0000:01:00.0"  is the PCI ID  of the device to  unbind from the
kernel nvme driver.

Tools
=====

Two simple applications  are provided with libnvme for  testing and as
example of  the library API  use.  The first application  is nvme_info
and allows getting information on a NVMe device.

    > nvme_info pci://0000:03:00.0
    Opening NVMe controller pci://0000:03:00.0
      Model name: INTEL SSDPEDMW400G4
      Serial number: CVCQ542500ZJ400AGN
      HW maximum queue entries: 4096
      Maximum queue depth: 1024
      Maximum request size: 128 KiB
    1 namespaces:
      Namespace 1/1: 512 bytes sectors, 781422768 sectors (372 GiB)
    31 I/O queue pairs:
      qpair 1/31: ID 1, max qd 1024, prio 0
      qpair 2/31: ID 2, max qd 1024, prio 0
      qpair 3/31: ID 3, max qd 1024, prio 0
      qpair 4/31: ID 4, max qd 1024, prio 0
      qpair 5/31: ID 5, max qd 1024, prio 0
      qpair 6/31: ID 6, max qd 1024, prio 0
      qpair 7/31: ID 7, max qd 1024, prio 0
      qpair 8/31: ID 8, max qd 1024, prio 0
      qpair 9/31: ID 9, max qd 1024, prio 0
      qpair 10/31: ID 10, max qd 1024, prio 0
      qpair 11/31: ID 11, max qd 1024, prio 0
      qpair 12/31: ID 12, max qd 1024, prio 0
      qpair 13/31: ID 13, max qd 1024, prio 0
      qpair 14/31: ID 14, max qd 1024, prio 0
      qpair 15/31: ID 15, max qd 1024, prio 0
      qpair 16/31: ID 16, max qd 1024, prio 0
      qpair 17/31: ID 17, max qd 1024, prio 0
      qpair 18/31: ID 18, max qd 1024, prio 0
      qpair 19/31: ID 19, max qd 1024, prio 0
      qpair 20/31: ID 20, max qd 1024, prio 0
      qpair 21/31: ID 21, max qd 1024, prio 0
      qpair 22/31: ID 22, max qd 1024, prio 0
      qpair 23/31: ID 23, max qd 1024, prio 0
      qpair 24/31: ID 24, max qd 1024, prio 0
      qpair 25/31: ID 25, max qd 1024, prio 0
      qpair 26/31: ID 26, max qd 1024, prio 0
      qpair 27/31: ID 27, max qd 1024, prio 0
      qpair 28/31: ID 28, max qd 1024, prio 0
      qpair 29/31: ID 29, max qd 1024, prio 0
      qpair 30/31: ID 30, max qd 1024, prio 0
      qpair 31/31: ID 31, max qd 1024, prio 0

The second application is a  simple benchmark tool allowing to quickly
measure   a   device   performance.   This   application   is   called
nvme_perf. An example  run is shown below (512B random  reads at queue
depth 1).

    > nvme_perf -ns 1 -qd 1 -rnd pci://0000:03:00.0 512
    Opening NVMe controller pci://0000:03:00.0
    Attached NVMe controller INTEL SSDPEDMW400G4 (CVCQ542500ZJ400AGN) (1 namespace)
    Qpair 1, depth: 1024
    Device 0000:03:00.0, namespace 1:
        372.611 GiB capacity (781422768 sectors of 512 B)
    Starting test on CPU 0 for 10 seconds:
        100 % read I/O, 0 % write I/Os
        512 B I/O size, random access, qd 1
    -> 2653607 I/Os in 10.000 secs
        135.865 MB/sec, 265360 IOPS
        3.768 usecs average I/O latency
    Detaching NVMe controller 0000:03:00.0

