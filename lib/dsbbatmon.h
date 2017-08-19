/*-
 * Copyright (c) 2017 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _DSBBATMON_H_
#define _DSBBATMON_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dev/acpica/acpiio.h>

#ifndef TEST
# define PATH_DEVD_SOCKET	 "/var/run/devd.pipe"
#else
# define PATH_DEVD_SOCKET	 "/tmp/dsbbatmon-test.socket"
# define PATH_TEST_UNIT_FILE	 "/tmp/dsbbatmon-test.units"
#endif
#define ACPIDEV			 "/dev/acpi"

#define DSBBATMON_ERR_SYS	 (1 << 0)
#define DSBBATMON_ERR_FATAL	 (1 << 1)
#define DSBBATMON_ERRBUF_SZ	 1024

typedef struct acpi_s {
	int cap;	/* Battery capacity */
	int status;	/* (Dis)charging, AC Power */
#define ACPI_STATUS_DISCHARGING  1
#define ACPI_STATUS_CHARGING	 2
#define ACPI_STATUS_ACLINE	 3
#define ACPI_STATUS_UNKNOWN	-1
	int acpifd;	/* Filedescriptor to ACPI */
	int min;	/* Minutes remaining */
} acpi_t;

typedef struct dsbbatmon_s {
	int     rd;
	int     bufsz;
	int     slen;
	int     socket;
	int	units;
	bool	conn_replaced;
	char    errmsg[DSBBATMON_ERRBUF_SZ];
	char   *lnbuf;
	acpi_t  acpi;
} dsbbatmon_t;

extern int	   dsbbatmon_init(dsbbatmon_t *);
extern int	   dsbbatmon_check_for_batt_event(dsbbatmon_t *);
extern int	   dsbbatmon_check_battery_presence(dsbbatmon_t *);
extern int	   dsbbatmon_poll(dsbbatmon_t *);
extern bool	   dsbbatmon_battery_present(dsbbatmon_t *);
extern bool	   dsbbatmon_devd_connection_replaced(dsbbatmon_t *bm);
extern const char *dsbbatmon_strerror(dsbbatmon_t *bm);
#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif	/* !_DSBBATMON_H_ */
