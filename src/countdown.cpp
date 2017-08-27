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
#include <QStyle>
#include <QDesktopWidget>

#include "countdown.h"
#include "qt-helper/qt-helper.h"

Countdown::Countdown(bool suspend, int seconds, QWidget *parent)
	: QDialog(parent)
{
	this->suspend = suspend;
	shutdownTime = time(NULL) + seconds;

	QIcon cnclIcon = qh_loadStockIcon(QStyle::SP_DialogCancelButton, 0);
	QIcon pic      = qh_loadStockIcon(QStyle::SP_MessageBoxCritical, 0);
	QIcon tIcon    = pic;

	label		    = new QLabel("");
	timer		    = new QTimer(this);
	QLabel	    *icon   = new QLabel;
	QHBoxLayout *hbox   = new QHBoxLayout;
	QVBoxLayout *layout = new QVBoxLayout(this);
	QPushButton *cancel = new QPushButton(cnclIcon, tr("&Cancel"));

	setLabelText(seconds);
	icon->setPixmap(pic.pixmap(64));

	setWindowIcon(tIcon);
	setWindowTitle(tr("Critical battery capacity reached"));

	hbox->setSpacing(20);
	hbox->addWidget(icon);
	hbox->addWidget(label, 1, Qt::AlignHCenter);

	layout->addLayout(hbox);
	layout->addWidget(cancel, 0, Qt::AlignRight);

	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
	show();
	setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
	    size(), qApp->desktop()->availableGeometry()));
	timer->start(1000);
}

void Countdown::setLabelText(int seconds)
{
	QString s;

	if (seconds <= 0)
		return;
	if (suspend)
		s = tr("Suspending system in %1 %2");
	else
		s = tr("Shutting down system in %1 %2");
	s = s.arg(seconds);
	s = s.arg(seconds == 1 ? tr("Second") : tr("Seconds"));
	label->setText(s);
}

void Countdown::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Escape)
		return;
	QDialog::keyPressEvent(e);
}

void Countdown::closeEvent(QCloseEvent *event)
{
	event->ignore();
}

void Countdown::update()
{
	int secondsLeft = (int)(shutdownTime - time(NULL));

	if (secondsLeft > 0) {
		timer->start(1000);
		setLabelText(secondsLeft);
	} else if (secondsLeft <= 0)
		accept();
}

