// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "network-web/discoverfeedsbutton.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"


DiscoverFeedsButton::DiscoverFeedsButton(QWidget *parent) : QToolButton(parent) {
  setEnabled(false);
  setIcon(qApp->icons()->fromTheme(QSL("folder-feed")));
  setPopupMode(QToolButton::InstantPopup);
}

DiscoverFeedsButton::~DiscoverFeedsButton() {
}

void DiscoverFeedsButton::clearFeedAddresses() {
  setFeedAddresses(QStringList());
}

void DiscoverFeedsButton::setFeedAddresses(const QStringList &addresses) {
  setEnabled(!addresses.isEmpty());
  setToolTip(addresses.isEmpty() ?
               tr("This website does not contain any feeds.") :
               tr("Click me to add feeds from this website.\nThis website contains %n feed(s).", 0, addresses.size()));

  if (menu() == NULL) {
    // Initialize the menu.
    setMenu(new QMenu(this));
    connect(menu(), SIGNAL(triggered(QAction*)), this, SLOT(linkTriggered(QAction*)));
  }

  menu()->hide();

  if (!addresses.isEmpty()) {
    menu()->clear();

    foreach (const QString &feed, addresses) {
      menu()->addAction(feed);
    }
  }
}

void DiscoverFeedsButton::linkTriggered(QAction *action) {
  emit addingOfFeedRequested(action->text());
}
