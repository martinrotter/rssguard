// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/statusbar.h"

#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "gui/plaintoolbutton.h"
#include "network-web/adblock/adblockicon.h"
#include "miscellaneous/iconfactory.h"

#include <QToolButton>
#include <QLabel>
#include <QProgressBar>
#include <QThread>


StatusBar::StatusBar(QWidget *parent) : QStatusBar(parent) {
  setSizeGripEnabled(false);
  setContentsMargins(0, 0, 0, 0);

  m_adBlockIcon = new AdBlockIcon(this);
  m_adBlockIcon->activate();

  // Initializations of widgets for status bar.
  m_fullscreenSwitcher = new PlainToolButton(this);
  m_fullscreenSwitcher->setCheckable(true);
  m_fullscreenSwitcher->setIcon(qApp->icons()->fromTheme(QSL("view-fullscreen")));
  m_fullscreenSwitcher->setText(tr("Fullscreen mode"));
  m_fullscreenSwitcher->setToolTip(tr("Switch application between fulscreen/normal states right from this status bar icon."));

  m_barProgressFeeds = new QProgressBar(this);
  m_barProgressFeeds->setTextVisible(false);
  m_barProgressFeeds->setFixedWidth(100);
  m_barProgressFeeds->setVisible(false);

  m_lblProgressFeeds = new QLabel(this);
  m_lblProgressFeeds->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_lblProgressFeeds->setVisible(false);

  m_barProgressDownload = new QProgressBar(this);
  m_barProgressDownload->setTextVisible(true);
  m_barProgressDownload->setFixedWidth(100);
  m_barProgressDownload->setVisible(false);

  m_lblProgressDownload = new QLabel(this);
  m_lblProgressDownload->setText("Downloading files in background");
  m_lblProgressDownload->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_lblProgressDownload->setVisible(false);

  m_lblProgressDownload->installEventFilter(this);
  m_barProgressDownload->installEventFilter(this);

  // Add widgets.
  addPermanentWidget(m_lblProgressFeeds);
  addPermanentWidget(m_barProgressFeeds);
  addPermanentWidget(m_lblProgressDownload);
  addPermanentWidget(m_barProgressDownload);
  addPermanentWidget(m_adBlockIcon);
  addPermanentWidget(m_fullscreenSwitcher);
}

StatusBar::~StatusBar() {
  qDebug("Destroying StatusBar instance.");
}

bool StatusBar::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_lblProgressDownload || watched == m_barProgressDownload) {
    if (event->type() == QEvent::MouseButtonPress) {
      qApp->mainForm()->tabWidget()->showDownloadManager();
    }
  }

  return false;
}

void StatusBar::showProgressFeeds(int progress, const QString &label) {
  m_lblProgressFeeds->setVisible(true);
  m_barProgressFeeds->setVisible(true);

  m_lblProgressFeeds->setText(label);
  m_barProgressFeeds->setValue(progress);
}

void StatusBar::clearProgressFeeds() {
  m_lblProgressFeeds->setVisible(false);
  m_barProgressFeeds->setVisible(false);
}

void StatusBar::showProgressDownload(int progress, const QString &tooltip) {
  m_lblProgressDownload->setVisible(true);
  m_barProgressDownload->setVisible(true);
  m_barProgressDownload->setValue(progress);
  m_barProgressDownload->setToolTip(tooltip);
  m_lblProgressDownload->setToolTip(tooltip);
}

void StatusBar::clearProgressDownload() {
  m_lblProgressDownload->setVisible(false);
  m_barProgressDownload->setVisible(false);
  m_barProgressDownload->setValue(0);
}
