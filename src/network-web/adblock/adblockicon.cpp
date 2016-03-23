// This file is part of RSS Guard.
//
// Copyright (C) 2014-2015 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#include "network-web/adblock/adblockicon.h"

#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/webpage.h"
#include "network-web/webbrowser.h"
#include "miscellaneous/application.h"
#include "gui/plaintoolbutton.h"
#include "gui/tabwidget.h"
#include "gui/dialogs/formmain.h"

#include <QMenu>
#include <QMouseEvent>


AdBlockIcon::AdBlockIcon(QWidget *window, QWidget *parent)
  : PlainToolButton(parent), m_window(window), m_menuAction(NULL), m_enabled(false) {
  connect(this, SIGNAL(clicked(QPoint)), this, SLOT(showMenu(QPoint)));
  connect(AdBlockManager::instance(), SIGNAL(enabledChanged(bool)), this, SLOT(setEnabled(bool)));
}

QAction *AdBlockIcon::menuAction() {
  if (m_menuAction == NULL) {
    m_menuAction = new QAction(tr("Adblock"), this);
    m_menuAction->setMenu(new QMenu(this));

    connect(m_menuAction->menu(), SIGNAL(aboutToShow()), this, SLOT(createMenu()));
  }

  m_menuAction->setIcon(m_enabled ? qApp->icons()->fromTheme("web-adblock") : qApp->icons()->fromTheme("web-adblock"));

  return m_menuAction;
}

void AdBlockIcon::activate() {
  setEnabled(AdBlockManager::instance()->shouldBeEnabled());
}

void AdBlockIcon::createMenu(QMenu *menu) {
  if (menu == NULL) {
    menu = qobject_cast<QMenu*>(sender());

    if (menu == NULL) {
      return;
    }
  }

  menu->clear();

  AdBlockManager *manager = AdBlockManager::instance();
  AdBlockCustomList *custom_list = manager->customList();

  WebPage *page = qApp->mainForm()->tabWidget()->currentWidget()->webBrowser()->view()->page();
  const QUrl page_url = page->url();

  menu->addAction(tr("Show Adblock &settings"), manager, SLOT(showDialog()));
  menu->addSeparator();

  if (!page_url.host().isEmpty() && m_enabled && manager->canRunOnScheme(page_url.scheme())) {
    const QString host = page_url.host().contains(QSL("www.")) ? page_url.host().mid(4) : page_url.host();
    const QString host_filter = QString(QSL("@@||%1^$document")).arg(host);
    const QString page_filter = QString(QSL("@@|%1|$document")).arg(page_url.toString());

    QAction *act;

    act = menu->addAction(tr("Disable on %1").arg(host));
    act->setCheckable(true);
    act->setChecked(custom_list->containsFilter(host_filter));
    act->setData(host_filter);
    connect(act, SIGNAL(triggered()), this, SLOT(toggleCustomFilter()));

    act = menu->addAction(tr("Disable only on this page"));
    act->setCheckable(true);
    act->setChecked(custom_list->containsFilter(page_filter));
    act->setData(page_filter);
    connect(act, SIGNAL(triggered()), this, SLOT(toggleCustomFilter()));

    menu->addSeparator();
  }
}

void AdBlockIcon::showMenu(const QPoint &pos) {
  QMenu menu;
  createMenu(&menu);
  menu.exec(pos);
}

void AdBlockIcon::toggleCustomFilter() {
  const QAction *action = qobject_cast<QAction*>(sender());

  if (action == NULL) {
    return;
  }

  const QString filter = action->data().toString();
  AdBlockManager *manager = AdBlockManager::instance();
  AdBlockCustomList *custom_list = manager->customList();

  if (custom_list->containsFilter(filter)) {
    custom_list->removeFilter(filter);
  }
  else {
    AdBlockRule *rule = new AdBlockRule(filter, custom_list);
    custom_list->addRule(rule);
  }
}

void AdBlockIcon::setEnabled(bool enabled) {
  if (enabled) {
    setToolTip(tr("Adblock - up and running"));
    setIcon(qApp->icons()->fromTheme("web-adblock"));
  }
  else {
    setToolTip(tr("Adblock - not running"));
    setIcon(qApp->icons()->fromTheme("web-adblock-disabled"));
  }

  m_enabled = enabled;
}

void AdBlockIcon::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    emit clicked(event->globalPos());
  }
  else {
    QToolButton::mouseReleaseEvent(event);
  }
}

AdBlockIcon::~AdBlockIcon() {
  qDebug("Destroying AdBlockIcon instance.");
}
