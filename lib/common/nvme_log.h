/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   Copyright (c) 2017, Western Digital Corporation or its affiliates.
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

#ifndef __NVME_LOG_H__
#define __NVME_LOG_H__

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * Log control structure.
 */
struct nvme_log {

	/*
	 * Log level.
	 */
	enum nvme_log_level	level;

	/*
	 * Log facility (output target)
	 */
	enum nvme_log_facility	facility;

	/*
	 * The output file for the NVME_LOG_FILE facility.
	 */
	FILE			*file;

};

/*
 * Generates a log message.
 * The message will be sent to the current log facility.
 * The level argument determines if the log should be displayed or
 * not, depending on the current library log level.
 */
extern void nvme_log(enum nvme_log_level level,
		     const char *format, ...)
#ifdef __GNUC__
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 2))
	__attribute__((cold))
#endif
#endif
	__attribute__((format(printf, 2, 3)));

/*
 * Generates a log message.
 * The message will be sent to the current log facility.
 * The level argument determines if the log should be displayed or
 * not, depending on the current library log level.
 */
extern void nvme_vlog(enum nvme_log_level level,
		      const char *format,
		      va_list ap)
	__attribute__((format(printf,2,0)));

/* System is unusable */
#define nvme_emerg(format, args...)		\
	nvme_log(NVME_LOG_EMERG,		\
		 "libnvme (FATAL): " format,	\
		 ## args)

/* Action must be taken immediately */
#define nvme_alert(format, args...)		\
	nvme_log(NVME_LOG_ALERT,		\
		 "libnvme (ALERT): " format,	\
		 ## args)

/* Critical conditions */
#define nvme_crit(format, args...)		\
	nvme_log(NVME_LOG_CRIT,			\
		 "libnvme (CRITICAL): " format,	\
		 ## args)

/* Error conditions */
#define nvme_err(format, args...)		\
	nvme_log(NVME_LOG_ERR,			\
		 "libnvme (ERROR): " format,	\
		 ## args)

/* Warning conditions */
#define nvme_warning(format, args...)		\
	nvme_log(NVME_LOG_WARNING,		\
		 "libnvme (WARNING): " format,	\
		 ## args)

/* Normal but significant condition */
#define nvme_notice(format, args...)	\
	nvme_log(NVME_LOG_NOTICE,	\
		 "libnvme: " format,	\
		 ## args)

/* Informational */
#define nvme_info(format, args...)	\
	nvme_log(NVME_LOG_INFO,		\
		 "libnvme: " format,	\
		 ## args)

/* Debug-level messages */
#define nvme_debug(format, args...)	\
	nvme_log(NVME_LOG_DEBUG,	\
		 "libnvme: " format,	\
		 ## args)

#endif /* __NVME_LOG_H__ */
