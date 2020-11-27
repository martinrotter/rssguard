// For license of this file, see <project-root-folder>/LICENSE.md.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/dialogs/formmain.h"
#include "gui/webbrowser.h"
#include "gui/webviewer.h"
#include "miscellaneous/application.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/webpage.h"

#include <QMenu>
#include <QMessageBox>
#include <QTimer>

AdBlockIcon::AdBlockIcon(AdBlockManager* parent)
  : QAction(parent), m_manager(parent), m_flashTimer(nullptr), m_timerTicks(0), m_enabled(m_manager->isEnabled()) {
  setToolTip(tr("AdBlock lets you block unwanted content on web pages"));
  setText(QSL("AdBlock"));
  setMenu(new QMenu());
  setIcon(m_enabled ? qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE) : qApp->icons()->miscIcon(ADBLOCK_ICON_DISABLED));

  connect(m_manager, &AdBlockManager::enabledChanged, this, &AdBlockIcon::setEnabled);
  connect(menu(), &QMenu::aboutToShow, this, [this]() {
    createMenu();
  });
  connect(this, &QAction::triggered, m_manager, &AdBlockManager::showDialog);
}

AdBlockIcon::~AdBlockIcon() {
  for (int i = 0; i < m_blockedPopups.count(); ++i) {
    delete m_blockedPopups.at(i).first;
  }

  if (menu() != nullptr) {
    menu()->deleteLater();
  }
}

void AdBlockIcon::popupBlocked(const QString& ruleString, const QUrl& url) {
  int index = ruleString.lastIndexOf(QLatin1String(" ("));
  const QString subscriptionName = ruleString.left(index);
  const QString filter = ruleString.mid(index + 2, ruleString.size() - index - 3);
  AdBlockSubscription* subscription = m_manager->subscriptionByName(subscriptionName);

  if (filter.isEmpty() || (subscription == nullptr)) {
    return;
  }

  QPair<AdBlockRule*, QUrl> pair;

  pair.first = new AdBlockRule(filter, subscription);
  pair.second = url;
  m_blockedPopups.append(pair);
  qApp->showGuiMessage(tr("Blocked popup window"), tr("AdBlock blocked unwanted popup window."), QSystemTrayIcon::Information);

  if (m_flashTimer == nullptr) {
    m_flashTimer = new QTimer(this);
  }

  if (m_flashTimer->isActive()) {
    stopAnimation();
  }

  m_flashTimer->setInterval(500);
  m_flashTimer->start();
  connect(m_flashTimer, &QTimer::timeout, this, &AdBlockIcon::animateIcon);
}

void AdBlockIcon::createMenu(QMenu* menu) {
  if (menu == nullptr) {
    menu = qobject_cast<QMenu*>(sender());

    if (menu == nullptr) {
      return;
    }
  }

  menu->clear();
  AdBlockCustomList* customList = m_manager->customList();
  WebPage* page = qApp->mainForm()->tabWidget()->currentWidget()->webBrowser()->viewer()->page();
  const QUrl pageUrl = page->url();

  menu->addAction(tr("Show AdBlock &settings"), m_manager, &AdBlockManager::showDialog);
  menu->addSeparator();

  if (!pageUrl.host().isEmpty() && m_enabled && m_manager->canRunOnScheme(pageUrl.scheme())) {
    const QString host = page->url().host().contains(QLatin1String("www.")) ? pageUrl.host().mid(4) : pageUrl.host();
    const QString hostFilter = QString("@@||%1^$document").arg(host);
    const QString pageFilter = QString("@@|%1|$document").arg(pageUrl.toString());
    QAction* act = menu->addAction(tr("Disable on %1").arg(host));

    act->setCheckable(true);
    act->setChecked(customList->containsFilter(hostFilter));
    act->setData(hostFilter);
    connect(act, &QAction::triggered, this, &AdBlockIcon::toggleCustomFilter);
    act = menu->addAction(tr("Disable only on this page"));
    act->setCheckable(true);
    act->setChecked(customList->containsFilter(pageFilter));
    act->setData(pageFilter);
    connect(act, &QAction::triggered, this, &AdBlockIcon::toggleCustomFilter);
    menu->addSeparator();
  }
}

void AdBlockIcon::showMenu(const QPoint& pos) {
  QMenu menu;

  createMenu(&menu);
  menu.exec(pos);
}

void AdBlockIcon::toggleCustomFilter() {
  auto* action = qobject_cast<QAction*>(sender());

  if (action == nullptr) {
    return;
  }

  const QString filter = action->data().toString();
  AdBlockCustomList* customList = m_manager->customList();

  if (customList->containsFilter(filter)) {
    customList->removeFilter(filter);
  }
  else {
    auto* rule = new AdBlockRule(filter, customList);

    customList->addRule(rule);
  }
}

void AdBlockIcon::animateIcon() {
  ++m_timerTicks;

  if (m_timerTicks > 10) {
    stopAnimation();
    return;
  }

  if (icon().isNull()) {
    setIcon(qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE));
  }
  else {
    setIcon(QIcon());
  }
}

void AdBlockIcon::stopAnimation() {
  m_timerTicks = 0;
  m_flashTimer->stop();
  disconnect(m_flashTimer, &QTimer::timeout, this, &AdBlockIcon::animateIcon);
  setEnabled(m_enabled);
}

void AdBlockIcon::setEnabled(bool enabled) {
  if (enabled) {
    setIcon(qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE));
  }
  else {
    setIcon(qApp->icons()->miscIcon(ADBLOCK_ICON_DISABLED));
  }

  m_enabled = enabled;
}
