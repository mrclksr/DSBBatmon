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

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include "acpi.h"

#ifdef TEST
char *
read_cmd()
{
	static int init = 1;
	static char buf[128];

	if (init) {
		setvbuf(stdin, (char *)NULL, _IONBF, 0);
		if (fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin),
		    F_GETFL) | O_NONBLOCK) == -1) {
			err(EXIT_FAILURE, "fcntl()");
		}
		init = 0;
	}
	if (fgets(buf, sizeof(buf) - 1, stdin) != NULL)
		return (buf);
	return (NULL);
}
#endif

int
init_acpi(acpi_t *acpi)
{
#ifdef TEST
	return (poll_acpi(acpi));
#endif
	if ((acpi->acpifd = open(ACPIDEV, O_RDONLY)) == -1) {
		warn("open(%s)", ACPIDEV);
		return (-1);
	}
	return (poll_acpi(acpi));
}

int
poll_acpi(acpi_t *acpi)
{
	int	     ac;
	static union acpi_battery_ioctl_arg battio;
#ifdef TEST
	static time_t t0 = 0;
	char *p;

	int d = rand() % 5;
	if (t0 == 0) {
		t0 = time(NULL);
		acpi->cap = 60;
		acpi->status = ACPI_STATUS_DISCHARGING;
	}
	if ((p = read_cmd()) != NULL)
		switch (p[0]) {
		case 'a':
			acpi->status = ACPI_STATUS_ACLINE;
			break;
		case 'c':
			acpi->status = ACPI_STATUS_CHARGING;
			break;
		case 'd':
			acpi->status = ACPI_STATUS_DISCHARGING;
		}
	if (time(NULL) - t0 >= 10) {
		if (acpi->status == ACPI_STATUS_DISCHARGING)
			acpi->cap -= d;
		else if (acpi->status == ACPI_STATUS_CHARGING)
			acpi->cap += d;
		else if (acpi->status == ACPI_STATUS_ACLINE)
			return (0);
		if (acpi->cap <= 0)
			acpi->cap = 0;
		else if (acpi->cap >= 100) {
			acpi->cap = 100;
			acpi->status = ACPI_STATUS_ACLINE;
		}
		t0 = time(NULL);
	}
	acpi->min = 100;

	return (0);
#endif
	battio.unit = 0;
	if (ioctl(acpi->acpifd, ACPIIO_BATT_GET_BATTINFO, &battio) == -1) {
		warn("ioctl(ACPIIO_BATT_GET_BATTINFO)");
		return (-1);
	}
	if (ioctl(acpi->acpifd, ACPIIO_ACAD_GET_STATUS, &ac) == -1) {
		warn("ioctl(ACPIIO_ACAD_GET_STATUS)");
		return (-1);
	}
	acpi->cap = battio.battinfo.cap;
	acpi->min = battio.battinfo.min;

	if (battio.battinfo.state & ACPI_BATT_STAT_DISCHARG)
		acpi->status = ACPI_STATUS_DISCHARGING;
	else if (battio.battinfo.state & ACPI_BATT_STAT_CHARGING)
		acpi->status = ACPI_STATUS_CHARGING;
	else if (ac > 0)
		acpi->status = ACPI_STATUS_ACLINE;
	else
		acpi->status = ACPI_STATUS_UNKNOWN;
	return (0);
}
