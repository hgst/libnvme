#!/usr/bin/env bash

set -e

function configure_linux {
	if mount | grep -qv hugetlbfs; then
		mkdir -p /mnt/huge
		mount -t hugetlbfs nodev /mnt/huge
	fi

	echo $NRHUGE > /proc/sys/vm/nr_hugepages
}

if [ "$1" = "" ]; then
	NRHUGE=128
else
	NRHUGE="$1"
fi

configure_linux

