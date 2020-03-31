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

#include "nvme_common.h"

#include <sys/types.h>
#include <syslog.h>

/*
 * Log control structure: initialize to default early log,
 * which is stdout output and INFO level for all log types.
 */
static struct nvme_log log = {
	.level = NVME_LOG_NOTICE,
	.facility = NVME_LOG_STDOUT,
	.file = NULL,
};

/*
 * Close the current log facility.
 */
static void nvme_close_log(void)
{

	switch (log.facility) {
	case NVME_LOG_FILE:
		if (log.file) {
			fflush(log.file);
			fclose(log.file);
			log.file = NULL;
		}
		break;
	case NVME_LOG_SYSLOG:
		closelog();
		break;
	case NVME_LOG_STDOUT:
	default:
		break;
	}

	log.facility = NVME_LOG_STDOUT;
}

/*
 * Send a log message to syslog.
 */
static void nvme_vlog_syslog(enum nvme_log_level level, const char *format,
			     va_list ap)
{
	char buf[BUFSIZ + 1];
	int ret;

	/* Generate message in buf */
	ret = vsnprintf(buf, BUFSIZ, format, ap);
	if (ret > 0) {
		buf[ret] = '\0';
		syslog(log.level - 1, "%s", buf);
	}
}

/*
 * Open a new log facility.
 */
int nvme_set_log_facility(enum nvme_log_facility facility, const char *path)
{
	int ret = 0;

	/* Close current log */
	nvme_close_log();

	switch (facility) {
	case NVME_LOG_STDOUT:
		/* Nothing to do */
		break;
	case NVME_LOG_FILE:
		log.file = fopen(path, "w+");
		if (!log.file) {
			ret = -errno;
			goto out;
		}
		break;
	case NVME_LOG_SYSLOG:
		openlog(path, LOG_NDELAY | LOG_PID, LOG_USER);
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		goto out;
	}

	log.facility = facility;

out:

	if (ret != 0) {
		/* Fallback to default on error */
		log.facility = NVME_LOG_STDOUT;
		log.file = NULL;
	}

	return ret;
}

/*
 * Get log facility.
 */
enum nvme_log_facility nvme_get_log_facility(void)
{
	return log.facility;
}

/*
 * Set log level.
 */
void nvme_set_log_level(enum nvme_log_level level)
{
	if ((level >= NVME_LOG_EMERG) && (level <= NVME_LOG_DEBUG))
		log.level = level;
}

/*
 * Get log level.
 */
enum nvme_log_level nvme_get_log_level(void)
{
	return log.level;
}

/*
 * Generates a log message.
 */
void nvme_vlog(enum nvme_log_level level, const char *format, va_list ap)
{
	FILE *f = log.file;

	if (level > log.level)
		return;

	switch(log.facility) {
	case NVME_LOG_STDOUT:
		f = stdout;
		/* fallthru */
	case NVME_LOG_FILE:
		if (f) {
			(void)vfprintf(f, format, ap);
			fflush(f);
		}
		break;
	case NVME_LOG_SYSLOG:
		nvme_vlog_syslog(level, format, ap);
		break;
	}
}

/*
 * Generates a log message.
 */
void nvme_log(enum nvme_log_level level, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	nvme_vlog(level, format, ap);
	va_end(ap);
}
