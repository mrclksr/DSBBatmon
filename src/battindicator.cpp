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

#include <QTimer>
#include <QSystemTrayIcon>
#include <unistd.h>
#include "countdown.h"
#include "preferences.h"
#include "battindicator.h"
#include "qt-helper/qt-helper.h"

BattIndicator::BattIndicator(dsbcfg_t *cfg, QWidget *parent) :
	QWidget(parent) {
	trayTimer = new QTimer(this);
	QTimer *pollTimer = new QTimer(this);

	this->cfg = cfg; acpi_prev.cap = 0;
	shutdown = shutdownCanceled = false;

	if (init_acpi(&acpi) == -1)
		qh_err(0, EXIT_FAILURE, "init_acpi()");
	loadIcons();
	updateSettings();
	connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollACPI()));
	connect(trayTimer, SIGNAL(timeout()), this, SLOT(checkForSysTray()));
        trayTimer->start(500);
	pollTimer->start(5000);
}

void BattIndicator::updateSettings()
{
	capShutdown  = dsbcfg_getval(cfg, CFG_CAP_SHUTDOWN).integer;
	autoShutdown = dsbcfg_getval(cfg, CFG_AUTOSHUTDOWN).boolean;

	if (acpi.cap > capShutdown)
		shutdownCanceled = false;
}

void BattIndicator::loadIcons()
{
	dBattIcon[4] = qh_loadIcon("battery-full-symbolic", NULL);
	dBattIcon[3] = qh_loadIcon("battery-good-symbolic", NULL);
	dBattIcon[2] = qh_loadIcon("battery-low-symbolic", NULL);
	dBattIcon[1] = qh_loadIcon("battery-caution-symbolic", NULL);
	dBattIcon[0] = qh_loadIcon("battery-empty-symbolic", NULL);

	cBattIcon[4] = qh_loadIcon("battery-full-charging-symbolic", NULL);
	cBattIcon[3] = qh_loadIcon("battery-good-charging-symbolic", NULL);
	cBattIcon[2] = qh_loadIcon("battery-low-charging-symbolic", NULL);
	cBattIcon[1] = qh_loadIcon("battery-caution-charging-symbolic", NULL);
	cBattIcon[0] = qh_loadIcon("battery-empty-charging-symbolic", NULL);

	acIcon = qh_loadIcon("battery-full-charged-symbolic", NULL);
}

void BattIndicator::trayClicked(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || 
	    reason == QSystemTrayIcon::DoubleClick) {
		if (!shutdown)
			showConfigMenu();
	}
}

void BattIndicator::updateIcon()
{
	int i;
	if (acpi.cap >= 90)
		i = 4;
	else if (acpi.cap < 5)
		i = 0;
	else if (acpi.cap < 20)
		i = 1;
	else if (acpi.cap < 40)
		i = 2;
	else
		i = 3;
	if (acpi.status == ACPI_STATUS_DISCHARGING)
		trayIcon->setIcon(dBattIcon[i]);
	else if (acpi.status == ACPI_STATUS_CHARGING)
		trayIcon->setIcon(cBattIcon[i]);
	else
		trayIcon->setIcon(acIcon);
}

void BattIndicator::updateToolTip()
{
	QString tt;

	if (acpi.status == ACPI_STATUS_CHARGING)
		tt = QString(tr("Status: Charging\nCapacity: %1%"));
	else if (acpi.status == ACPI_STATUS_DISCHARGING)
		tt = QString(tr("Status: Discharging\nCapacity: %1%"));
	else if (acpi.status == ACPI_STATUS_ACLINE)
		tt = QString(tr("Status: Running on AC power"));
	if (acpi.status != ACPI_STATUS_ACLINE)
		tt = tt.arg(acpi.cap);
	if (acpi.min != -1 && acpi.status != ACPI_STATUS_ACLINE) {
		int hrs = acpi.min / 60;
		int min = acpi.min % 60;
		tt = tt.append(tr("\nTime remaining: %1:%2")).arg(hrs).arg(min);
	}		
	trayIcon->setToolTip(tt);
}

void BattIndicator::pollACPI()
{
	(void)poll_acpi(&acpi);

	if (acpi.cap != acpi_prev.cap || acpi.min != acpi_prev.min ||
	    acpi.status != acpi_prev.status) {
		updateIcon(); updateToolTip();
		if (acpi.status != ACPI_STATUS_DISCHARGING) {
			shutdown = shutdownCanceled = false;
		} else if (!shutdownCanceled && autoShutdown &&
		    acpi.cap <= capShutdown) {
			shutdown = true;
			showShutdownWin();
		} else if (acpi_prev.cap <= 0 && acpi.cap < 40) {
			showWarnMsg();
		} else if ((acpi_prev.cap >= 40 && acpi.cap < 40) ||
		    (acpi_prev.cap >= 20 && acpi.cap < 20) ||
		    (acpi_prev.cap >= 5 && acpi.cap < 5)) {
			showWarnMsg();
		}
		acpi_prev = acpi;
	}
}

QMenu *BattIndicator::createTrayMenu()
{
	QIcon quitIcon  = qh_loadIcon("application-exit", NULL);
	QIcon prefsIcon = qh_loadIcon("preferences-system", NULL);
	QMenu   *menu = new QMenu(this);
	QAction *quitAction = new QAction(quitIcon, tr("&Quit"), this);
        QAction *preferencesAction = new QAction(prefsIcon,
	    tr("&Preferences"), this);

	menu->addAction(preferencesAction);
	menu->addAction(quitAction);
	connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));
	connect(preferencesAction, SIGNAL(triggered()), this,
	    SLOT(showConfigMenu()));
	return (menu);
}

void BattIndicator::showConfigMenu()
{
	Preferences prefs(cfg);

	if (prefs.exec() == (int)QDialog::Accepted)
		updateSettings();
}

void BattIndicator::showShutdownWin()
{
	bool doSuspend = dsbcfg_getval(cfg, CFG_SUSPEND).boolean;

	Countdown countdownWin(doSuspend, 30, this);
	if (countdownWin.exec() == QDialog::Rejected) {
		shutdown = false;
		shutdownCanceled = true;
	} else {
		const char *cmd = doSuspend ? \
		    dsbcfg_getval(cfg, CFG_SUSPEND_CMD).string : \
		    dsbcfg_getval(cfg, CFG_SHUTDOWN_CMD).string;

		dsbcfg_write(PROGRAM, "config", cfg);

		(void)execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
		qh_err(0, EXIT_FAILURE, "Couldn't execute %", cmd);
	}
}

void BattIndicator::showWarnMsg()
{
	QString msg = QString(tr("Battery capacity at %1%")).arg(acpi.cap);
	trayIcon->showMessage(QString(tr("Warning")), msg,
	    acpi.cap <= 5 ? QSystemTrayIcon::Critical : \
	    QSystemTrayIcon::Warning, 20000);
}

void BattIndicator::checkForSysTray()
{
	static int tries = 60;

	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		trayTimer->stop();
		createTrayIcon();
	} else if (tries-- <= 0) {
		trayTimer->stop();
		show();
	}
}

void BattIndicator::createTrayIcon()
{
	trayIcon    = new QSystemTrayIcon(this);
	QMenu *menu = createTrayMenu();

	updateIcon();
	updateToolTip();
	trayIcon->setContextMenu(menu);
	trayIcon->show();
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	    this, SLOT(trayClicked(QSystemTrayIcon::ActivationReason)));
}

void BattIndicator::quit()
{
	dsbcfg_write(PROGRAM, "config", cfg);
	exit(EXIT_SUCCESS);
}

