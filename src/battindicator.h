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

#include <QWidget>
#include <QDialog>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>
#include <time.h>
#include "lib/acpi.h"
#include "lib/config.h"

class BattIndicator : public QWidget
{
	Q_OBJECT
public:
	BattIndicator(dsbcfg_t *cfg, QWidget *parent = 0);
private slots:
	void quit();
	void pollACPI();
	void showConfigMenu();
	void trayClicked(QSystemTrayIcon::ActivationReason reason);
	void checkForSysTray();
private:
	void showShutdownWin();
	void updateToolTip();
	void updateIcon();
	void showWarnMsg();
	void updateSettings();
	void loadIcons();
	void createTrayIcon();
	QIcon createIcon(int status);
	QMenu *createTrayMenu();

	int  capShutdown;
	int  lastCap = -1;
	bool missingIcon = false;
	bool shutdown;
	bool useIconTheme;
	bool autoShutdown;
	bool shutdownCanceled;
	QIcon cBattIcon[5];
	QIcon dBattIcon[5];
	QIcon acIcon;
	acpi_t acpi;
	acpi_t acpi_prev;
	dsbcfg_t *cfg;
	QTimer *trayTimer;
	QSystemTrayIcon *trayIcon;
};

