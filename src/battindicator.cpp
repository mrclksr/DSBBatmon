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
#include <QPainter>
#include <QPointer>
#include <err.h>
#include <unistd.h>
#include "countdown.h"
#include "preferences.h"
#include "battindicator.h"
#include "qt-helper/qt-helper.h"

BattIndicator::BattIndicator(dsbcfg_t *cfg, QWidget *parent) :
	QWidget(parent) {
	bm        = new dsbbatmon_t;
	trayTimer = new QTimer(this);
	pollTimer = new QTimer(this);
	this->cfg = cfg;

	loadIcons(); updateSettings();
	switch (dsbbatmon_init(bm)) {
	case  0:
		/* There is no battery slot. */
		errx(EXIT_FAILURE, "No battery slot installed.");
	case -1:
		qh_err(0, EXIT_FAILURE, "dsbbatmon_init(): %s",
		    dsbbatmon_strerror(bm));
	}
	initSocketNotifier(bm->socket);

	connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollACPI()));
	connect(trayTimer, SIGNAL(timeout()), this, SLOT(checkForSysTray()));

	if (dsbbatmon_battery_present(bm)) {
		pollTimer->start(pollInterval * 1000);
		trayTimer->start(500);
	} else {
		qDebug("Battery not present. Waiting ...");
	}
}

void BattIndicator::initSocketNotifier(int socket)
{
	swatcher = new QSocketNotifier(socket, QSocketNotifier::Read, this);

	connect(swatcher, SIGNAL(activated(int)), this,
	    SLOT(catchActivated(int)));
}

void BattIndicator::updateSettings()
{
	doSuspend    = dsbcfg_getval(cfg, CFG_SUSPEND).boolean;
	capShutdown  = dsbcfg_getval(cfg, CFG_CAP_SHUTDOWN).integer;
	pollInterval = dsbcfg_getval(cfg, CFG_POLL_INTERVAL).integer;
	autoShutdown = dsbcfg_getval(cfg, CFG_AUTOSHUTDOWN).boolean;

	if (doSuspend)
		command = dsbcfg_getval(cfg, CFG_SUSPEND_CMD).string;
	else
		command = dsbcfg_getval(cfg, CFG_SHUTDOWN_CMD).string;
	if (useIconTheme != dsbcfg_getval(cfg, CFG_USE_ICON_THEME).boolean) {
		useIconTheme = dsbcfg_getval(cfg, CFG_USE_ICON_THEME).boolean;
		loadIcons(); updateIcon();
	}
	if (bm->acpi.cap > capShutdown)
		shutdownCanceled = false;
	pollTimer->start(pollInterval * 1000);
}

QIcon BattIndicator::createIcon(int status)
{
	QColor  color;
	QColor  charge(CHARGE_COLOR);
	QColor  discharge(DISCHARGE_COLOR);
	QColor  critical(CRITICAL_COLOR);
	QPixmap pix(10, 24);

	pix.fill(QColor(0, 0, 0, 0));
	QPainter p(&pix);

	if (status < 0 && -status < 20)
		color = critical;
	else if (status < 0)
		color = discharge;
	else
		color = charge;
	int x = (21 * status / 100);
	if (x < 0)
		x *= -1;
	p.setPen(color);
	p.drawRect(4, 0, 2, 1);
	p.drawRect(0, 1, 9, 22);
	p.fillRect(QRectF(1, 22 - x, 8, x + 1), color);
	QIcon icon(pix);

	return (icon);
}

void BattIndicator::loadIcons()
{
	missingIcon = false;

	dBattIcon[4] = qh_loadIcon("battery-full-symbolic",
				   "battery", NULL);
	dBattIcon[3] = qh_loadIcon("battery-good-symbolic",
				   "battery", NULL);
	dBattIcon[2] = qh_loadIcon("battery-low-symbolic",
				   "battery-low", NULL);
	dBattIcon[1] = qh_loadIcon("battery-caution-symbolic",
			           "battery-caution", NULL);
	dBattIcon[0] = qh_loadIcon("battery-empty-symbolic",
				   "battery-low", NULL);
	cBattIcon[4] = qh_loadIcon("battery-full-charging-symbolic",
				   "battery", NULL);
	cBattIcon[3] = qh_loadIcon("battery-good-charging-symbolic",
				   "battery", NULL);
	cBattIcon[2] = qh_loadIcon("battery-low-charging-symbolic",
				   "battery-low", NULL);
	cBattIcon[1] = qh_loadIcon("battery-caution-charging-symbolic",
				   "battery-caution", NULL);
	cBattIcon[0] = qh_loadIcon("battery-empty-charging-symbolic",
				   "battery-low", NULL);
	acIcon	     = qh_loadIcon("battery-full-charged-symbolic",
				   "battery", NULL);
	quitIcon     = qh_loadIcon("application-exit", NULL);
	prefsIcon    = qh_loadIcon("preferences-system", NULL);

	for (int i = 0; i < 5 && !missingIcon; i++) {
		if (dBattIcon[i].isNull())
			missingIcon = true;
		if (cBattIcon[i].isNull())
			missingIcon = true;
	}
	if (quitIcon.isNull())
		quitIcon = qh_loadStockIcon(QStyle::SP_DialogCloseButton);
	if (prefsIcon.isNull())
		prefsIcon = qh_loadStockIcon(QStyle::SP_FileIcon);
}

void BattIndicator::trayClicked(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || 
	    reason == QSystemTrayIcon::DoubleClick) {
		if (!shutdown)
			showConfigMenu();
	}
}

void BattIndicator::catchActivated(int /*socket*/)
{
	switch (dsbbatmon_check_for_batt_event(bm)) {
	case  0:
		if (!dsbbatmon_devd_connection_replaced(bm))
			return;
		delete swatcher;
		initSocketNotifier(bm->socket);
		return;
	case -1:
		qh_errx(0, EXIT_FAILURE, "%s", dsbbatmon_strerror(bm));
	}
	update();
}

void BattIndicator::update()
{
	if (dsbbatmon_poll(bm) == -1)
		qh_err(0, EXIT_FAILURE, "%s", dsbbatmon_strerror(bm));
	if (dsbbatmon_battery_present(bm)) {
		showTrayIcon();
		pollTimer->start(pollInterval * 1000);
	} else {
		hideTrayIcon();
		pollTimer->stop();
	}
}

void BattIndicator::showTrayIcon()
{
	if (trayIcon == 0)
		createTrayIcon();
	else
		trayIcon->show();
	updateIcon(); updateToolTip();
}

void BattIndicator::hideTrayIcon()
{
	if (trayIcon != 0)
		trayIcon->hide();
}

void BattIndicator::updateIcon()
{
	int i;

	if (trayIcon == 0)
		return;
	if (missingIcon || !useIconTheme) {
		int status = bm->acpi.cap *
		    (bm->acpi.status == ACPI_STATUS_DISCHARGING ? -1 : 1);
		trayIcon->setIcon(createIcon(status));
		return;
	}
	if (bm->acpi.cap >= 90)
		i = 4;
	else if (bm->acpi.cap < 5)
		i = 0;
	else if (bm->acpi.cap < 20)
		i = 1;
	else if (bm->acpi.cap < 40)
		i = 2;
	else
		i = 3;
	if (bm->acpi.status == ACPI_STATUS_DISCHARGING) {
		trayIcon->setIcon(dBattIcon[i]);
	}
	else if (bm->acpi.status == ACPI_STATUS_CHARGING)
		trayIcon->setIcon(cBattIcon[i]);
	else
		trayIcon->setIcon(acIcon);
}

void BattIndicator::updateToolTip()
{
	QString tt;

	if (trayIcon == 0)
		return;
	if (bm->acpi.status == ACPI_STATUS_CHARGING)
		tt = QString(tr("Status: Charging\nCapacity: %1%"));
	else if (bm->acpi.status == ACPI_STATUS_DISCHARGING)
		tt = QString(tr("Status: Discharging\nCapacity: %1%"));
	else if (bm->acpi.status == ACPI_STATUS_ACLINE)
		tt = QString(tr("Status: Running on AC power"));
	else
		return;
	if (bm->acpi.status != ACPI_STATUS_ACLINE)
		tt = tt.arg(bm->acpi.cap);
	if (bm->acpi.min != -1 && bm->acpi.status != ACPI_STATUS_ACLINE) {
		int hrs = bm->acpi.min / 60;
		int min = bm->acpi.min % 60;
		tt = tt.append(tr("\nTime remaining: %1:%2"));
		tt = tt.arg(hrs, 2, 10, QChar('0'));
		tt = tt.arg(min, 2, 10, QChar('0'));
	}		
	trayIcon->setToolTip(tt);
}

void BattIndicator::pollACPI()
{
	
	if (!dsbbatmon_battery_present(bm))
		return;
	if (dsbbatmon_poll(bm) == -1) {
		qh_errx(0, EXIT_FAILURE, "dsbbatmon_poll(): %s",
		    dsbbatmon_strerror(bm));
	}
	if (bm->acpi.cap != acpi_prev.cap || bm->acpi.min != acpi_prev.min ||
	    bm->acpi.status != acpi_prev.status) {
		updateIcon(); updateToolTip();
		if (bm->acpi.status != ACPI_STATUS_DISCHARGING) {
			shutdown = shutdownCanceled = false;
		} else if (!shutdownCanceled && autoShutdown &&
		    bm->acpi.cap <= capShutdown && !shutdown) {
			if (!shutdownWinVisible) {
				shutdown = true;
				showShutdownWin();
			}
		} else if (acpi_prev.cap <= 0 && bm->acpi.cap < 40) {
			showWarnMsg();
		} else if ((acpi_prev.cap >= 40 && bm->acpi.cap < 40) ||
		    (acpi_prev.cap >= 20 && bm->acpi.cap < 20) ||
		    (acpi_prev.cap >= 5 && bm->acpi.cap < 5)) {
			showWarnMsg();
		}
		acpi_prev = bm->acpi;
	}
}

QMenu *BattIndicator::createTrayMenu()
{
	QMenu *menu		   = new QMenu(this);
	QAction *quitAction	   = new QAction(quitIcon, tr("&Quit"), this);
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
	QPointer<Preferences> prefs = new Preferences(cfg);
	if (prefs->exec() == (int)QDialog::Accepted)
		updateSettings();
	delete prefs;
}

void BattIndicator::showShutdownWin()
{
	int  ret;
	QPointer<Countdown> countdownWin = new Countdown(doSuspend, 30, this);

	shutdownWinVisible = true;
	if (countdownWin->exec() == QDialog::Rejected)
		shutdownCanceled = true;
	else {
		dsbcfg_write(PROGRAM, "config", cfg);
		if (!doSuspend) {
			/* Shutdown. Never return. */
			(void)execl("/bin/sh", "/bin/sh", "-c", command, NULL);
			qh_err(0, EXIT_FAILURE, "Couldn't execute %", command);
		} else {
			/* Suspend. Do not not exit */
			switch ((ret = system(command))) {
			case   0:
				break;
			case  -1:
				qh_err(0, EXIT_FAILURE, "system(%s)", command);
			case 127:
				qh_err(0, EXIT_FAILURE, "Failes to execute " \
				    "shell.");
			default:
				qh_warnx(0, "Command %s returned an exit " \
				    "status != 0: %d", ret);
			}
		}
	}
	delete countdownWin;
	shutdown = shutdownWinVisible = false;
}

void BattIndicator::showWarnMsg()
{
	QString msg = QString(tr("Battery capacity at %1%")).arg(bm->acpi.cap);
	trayIcon->showMessage(QString(tr("Warning")), msg,
	    bm->acpi.cap <= 5 ? QSystemTrayIcon::Critical : \
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

	updateIcon(); updateToolTip();
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

