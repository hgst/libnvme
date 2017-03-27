#
#  BSD LICENSE
#
#  Copyright (c) Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = . $(subdirs)

EXTRA_DIST = autogen.sh exports

AM_CPPFLAGS = \
	-O2 \
	-Wall -Wextra -Wno-unused-parameter \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/common \
	-I.
# -msse4.1 -mavx


pkgconfdir = $(libdir)/pkgconfig
pkgconf_DATA = libnvme.pc
pkginclude_HEADERS =

noinst_HEADERS =

bin_PROGRAMS =
noinst_PROGRAMS =

noinst_LTLIBRARIES = libnvme_common.la

include lib/common/Makemodule.am

lib_LTLIBRARIES = libnvme.la
libnvme_la_DEPENDENCIES = exports libnvme_common.la

include lib/nvme/Makemodule.am
include tools/perf/Makemodule.am
include tools/info/Makemodule.am
include include/Makemodule.am
