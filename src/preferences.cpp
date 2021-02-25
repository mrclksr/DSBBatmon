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

#include <QGroupBox>
#include <QDirIterator>

#include "preferences.h"
#include "qt-helper/qt-helper.h"

Preferences::Preferences(dsbcfg_t *cfg, QWidget *parent) : 
    QDialog(parent) {
	this->cfg = cfg;

	QIcon winIcon	 = qh_loadIcon("preferences-system", 0);
	QIcon okIcon	 = qh_loadStockIcon(QStyle::SP_DialogOkButton, 0);
	QIcon cancelIcon = qh_loadStockIcon(QStyle::SP_DialogCancelButton,
	    NULL);
	shutdownSb	     = new QSpinBox;
	pollIvalSb	     = new QSpinBox;
	autoShutdownCb	     = new QCheckBox(tr("Auto suspend/shutdown"));
	useIconThemeCb	     = new QCheckBox(tr("Use theme icons for tray"));
	container	     = new QWidget;
	shutdownCmd	     = new QLineEdit;
	suspendCmd	     = new QLineEdit;
	suspendRb	     = new QRadioButton(tr("Suspend"));
	shutdownRb	     = new QRadioButton(tr("Shutdown"));
	QGroupBox   *bhvgrp  = new QGroupBox(tr("Behavior"));
	QGroupBox   *viwgrp  = new QGroupBox(tr("Tray Icon Theme"));
	QFormLayout *form    = new QFormLayout;
	QFormLayout *form2   = new QFormLayout;
	QVBoxLayout *bhvbox  = new QVBoxLayout;
	QVBoxLayout *viwbox  = new QVBoxLayout;
	QHBoxLayout *hbox    = new QHBoxLayout;
	QHBoxLayout *bbox    = new QHBoxLayout;
	QVBoxLayout *vbox    = new QVBoxLayout(container);
	QVBoxLayout *layout  = new QVBoxLayout(this);
	QPushButton *ok	     = new QPushButton(okIcon, tr("&Ok"));
	QPushButton *cancel  = new QPushButton(cancelIcon, tr("&Cancel"));

	QLabel *str   = new QLabel(tr("if battery capacity drops below"));
	QLabel *slash = new QLabel("/");

	pollIvalSb->setMinimum(1);
	pollIvalSb->setSuffix(" s");
	shutdownSb->setSuffix(" %");

	shutdownCmd->insert(
	    QString(dsbcfg_getval(cfg, CFG_SHUTDOWN_CMD).string));
	suspendCmd->insert(
	    QString(dsbcfg_getval(cfg, CFG_SUSPEND_CMD).string));
	autoShutdownCb->setCheckState(
	    dsbcfg_getval(cfg, CFG_AUTOSHUTDOWN).boolean ? Qt::Checked : \
	    Qt::Unchecked);
	shutdownSb->setValue(dsbcfg_getval(cfg, CFG_CAP_SHUTDOWN).integer);
	pollIvalSb->setValue(dsbcfg_getval(cfg, CFG_POLL_INTERVAL).integer);

	if (autoShutdownCb->checkState() != Qt::Unchecked)
		container->setEnabled(true);
	else
		container->setEnabled(false);
	useIconThemeCb->setCheckState(
	    dsbcfg_getval(cfg, CFG_USE_ICON_THEME).boolean ? Qt::Checked : \
	    Qt::Unchecked);

	bbox->addWidget(ok, 1, Qt::AlignRight);
	bbox->addWidget(cancel, 0, Qt::AlignRight);

	hbox->addWidget(suspendRb, 0, Qt::AlignLeft);
	hbox->addWidget(slash, 0, Qt::AlignCenter);
	hbox->addWidget(shutdownRb, 0, Qt::AlignLeft);
	hbox->addWidget(str, 1, Qt::AlignLeft);
	hbox->addWidget(shutdownSb, 1, Qt::AlignRight);
	bhvbox->addWidget(container);

	suspendRb->setChecked(dsbcfg_getval(cfg, CFG_SUSPEND).boolean);
	shutdownRb->setChecked(!dsbcfg_getval(cfg, CFG_SUSPEND).boolean);
	vbox->addLayout(hbox);

	form->addRow(new QLabel(tr("Suspend command:")), suspendCmd);
	form->addRow(new QLabel(tr("Shutdown command:")), shutdownCmd);
	form->setLabelAlignment(Qt::AlignLeft);
	form->setContentsMargins(0, 0, 0, 0);
	vbox->setContentsMargins(0, 0, 0, 0);
	vbox->addLayout(form);

	bhvbox->addWidget(autoShutdownCb);

	form2->addRow(new QLabel(tr("ACPI poll interval:")), pollIvalSb);

	bhvbox->addLayout(form2);
	bhvgrp->setLayout(bhvbox);
	layout->addWidget(bhvgrp);

	createThemeComboBox();

	viwbox->addWidget(themeBox);
	viwbox->addWidget(useIconThemeCb);
	viwgrp->setLayout(viwbox);

	layout->addWidget(viwgrp);

	layout->addLayout(bbox);
	layout->setContentsMargins(15, 15, 15, 15);

	container->setContentsMargins(5, 5, 5, 5);

	vbox->addStretch(1);

	setWindowIcon(winIcon);
	setWindowTitle(tr("Preferences"));

	connect(ok, SIGNAL(clicked()), this, SLOT(acceptSlot()));
	connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
	connect(autoShutdownCb, SIGNAL(stateChanged(int)), this,
	    SLOT(catchCbStateChanged(int)));
}

void
Preferences::createThemeComboBox()
{
	themeBox	  = new QComboBox;
	QString curTheme  = QIcon::themeName();
	QStringList paths = QIcon::themeSearchPaths();
	QStringList names;
	QString themeName(dsbcfg_getval(cfg, CFG_TRAY_THEME).string);

	if (themeName.isNull())
		themeName = curTheme;
	for (int i = 0; i < paths.size(); i++) {
		QDirIterator it(paths.at(i));
		while (it.hasNext()) {
			QString indexPath = QString("%1/index.theme").arg(it.next());
			if (!it.fileInfo().isDir())
				continue;
			QString name = it.fileName();
			if (name == "." || name == "..")
				continue;
			QFile indexFile(indexPath);
			if (!indexFile.exists())
				continue;
			indexFile.close();
			names.append(name);
		}
	}
	names.sort(Qt::CaseInsensitive);
	names.removeDuplicates();
	themeBox->addItems(names);

	int index = themeBox->findText(themeName, Qt::MatchExactly);
	if (index != -1)
		themeBox->setCurrentIndex(index);
}

void
Preferences::catchCbStateChanged(int state)
{
	if (state == Qt::Checked)
		container->setEnabled(true);
	else if (state == Qt::Unchecked)
		container->setEnabled(false);
}

void
Preferences::acceptSlot()
{
	QByteArray suspendArr  = suspendCmd->text().toLatin1();
	QByteArray shutdownArr = shutdownCmd->text().toLatin1();

	dsbcfg_getval(cfg, CFG_AUTOSHUTDOWN).boolean =
		autoShutdownCb->checkState() == Qt::Checked ? true : false;
	dsbcfg_getval(cfg, CFG_CAP_SHUTDOWN).integer = shutdownSb->value();
	dsbcfg_getval(cfg, CFG_SUSPEND).boolean =
	    suspendRb->isChecked() ? true : false;
	dsbcfg_getval(cfg, CFG_POLL_INTERVAL).integer = pollIvalSb->value();

	dsbcfg_getval(cfg, CFG_USE_ICON_THEME).boolean =
		useIconThemeCb->checkState() == Qt::Checked ? true : false;

	if (strcmp(dsbcfg_getval(cfg, CFG_SUSPEND_CMD).string,
	    suspendArr.data()) != 0) {
		free(dsbcfg_getval(cfg, CFG_SUSPEND_CMD).string);
		if ((dsbcfg_getval(cfg, CFG_SUSPEND_CMD).string =
		    strdup(suspendArr.data())) == NULL)
			qh_err(0, EXIT_FAILURE, "strdup()");
	}
	if (strcmp(dsbcfg_getval(cfg, CFG_SHUTDOWN_CMD).string,
	    shutdownArr.data()) != 0) {
		free(dsbcfg_getval(cfg, CFG_SHUTDOWN_CMD).string);
		if ((dsbcfg_getval(cfg, CFG_SHUTDOWN_CMD).string =
		    strdup(shutdownArr.data())) == NULL)
			qh_err(0, EXIT_FAILURE, "strdup()");
	}
	dsbcfg_val_t val;
	val.string = themeBox->currentText().toLatin1().data();
	if (val.string != (char *)0 && *val.string != '\0')
		dsbcfg_setval(cfg, CFG_TRAY_THEME, val);
	dsbcfg_write(PROGRAM, "config", cfg);
	accept();
}

