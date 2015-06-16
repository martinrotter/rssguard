/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2015 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "network-web/adblock/adblockicon.h"

#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/webpage.h"
#include "network-web/webbrowser.h"

#include "miscellaneous/application.h"
#include "gui/plaintoolbutton.h"
#include "gui/formmain.h"
#include "gui/tabwidget.h"

#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <QWebFrame>


AdBlockIcon::AdBlockIcon(QWidget *window, QWidget *parent)
  : PlainToolButton(parent), m_window(window), m_menuAction(0), m_flashTimer(0), m_timerTicks(0), m_enabled(false) {
  setToolTip(tr("Adblock lets you block unwanted content on web pages."));

  connect(this, SIGNAL(clicked(QPoint)), this, SLOT(showMenu(QPoint)));
  connect(AdBlockManager::instance(), SIGNAL(enabledChanged(bool)), this, SLOT(setEnabled(bool)));
}

void AdBlockIcon::popupBlocked(const QString &ruleString, const QUrl &url)
{
  int index = ruleString.lastIndexOf(QLatin1String(" ("));

  const QString subscriptionName = ruleString.left(index);
  const QString filter = ruleString.mid(index + 2, ruleString.size() - index - 3);
  AdBlockSubscription* subscription = AdBlockManager::instance()->subscriptionByName(subscriptionName);
  if (filter.isEmpty() || !subscription) {
    return;
  }

  QPair<AdBlockRule*, QUrl> pair;
  pair.first = new AdBlockRule(filter, subscription);
  pair.second = url;
  m_blockedPopups.append(pair);

  //!** FIXME
  //    mApp->desktopNotifications()->showNotification(QPixmap(":images/images/adblock_big.png"), tr("Blocked popup window"), tr("AdBlock blocked unwanted popup window."));

  if (!m_flashTimer) {
    m_flashTimer = new QTimer(this);
  }

  if (m_flashTimer->isActive()) {
    stopAnimation();
  }

  m_flashTimer->setInterval(500);
  m_flashTimer->start();

  connect(m_flashTimer, SIGNAL(timeout()), this, SLOT(animateIcon()));
}

QAction* AdBlockIcon::menuAction()
{
  if (!m_menuAction) {
    m_menuAction = new QAction(tr("AdBlock"), this);
    m_menuAction->setMenu(new QMenu);
    connect(m_menuAction->menu(), SIGNAL(aboutToShow()), this, SLOT(createMenu()));
  }

  m_menuAction->setIcon(QIcon(m_enabled ? ":images/images/adblock.png" : ":images/images/adblock-disabled.png"));

  return m_menuAction;
}

void AdBlockIcon::createMenu(QMenu *menu)
{
  if (menu == NULL) {
    menu = qobject_cast<QMenu*>(sender());

    if (menu == NULL) {
      return;
    }
  }

  menu->clear();

  AdBlockManager *manager = AdBlockManager::instance();
  AdBlockCustomList *customList = manager->customList();

  WebPage* page = qApp->mainForm()->tabWidget()->widget(qApp->mainForm()->tabWidget()->currentIndex())->webBrowser()->view()->page();
  const QUrl pageUrl = page->mainFrame()->url();

  menu->addAction(tr("Show Adblock &settings"), manager, SLOT(showDialog()));
  menu->addSeparator();

  if (!pageUrl.host().isEmpty() && m_enabled && manager->canRunOnScheme(pageUrl.scheme())) {
    const QString host = pageUrl.host().contains(QLatin1String("www.")) ? pageUrl.host().mid(4) : pageUrl.host();
    const QString hostFilter = QString("@@||%1^$document").arg(host);
    const QString pageFilter = QString("@@|%1|$document").arg(pageUrl.toString());

    QAction* act = menu->addAction(tr("Disable on %1").arg(host));
    act->setCheckable(true);
    act->setChecked(customList->containsFilter(hostFilter));
    act->setData(hostFilter);
    connect(act, SIGNAL(triggered()), this, SLOT(toggleCustomFilter()));

    act = menu->addAction(tr("Disable only on this page"));
    act->setCheckable(true);
    act->setChecked(customList->containsFilter(pageFilter));
    act->setData(pageFilter);
    connect(act, SIGNAL(triggered()), this, SLOT(toggleCustomFilter()));

    menu->addSeparator();
  }

  if (!m_blockedPopups.isEmpty()) {
    menu->addAction(tr("Blocked Popup Windows"))->setEnabled(false);
    for (int i = 0; i < m_blockedPopups.count(); i++) {
      const QPair<AdBlockRule*, QUrl> &pair = m_blockedPopups.at(i);

      QString address = pair.second.toString().right(55);
      QString actionText = tr("%1 with (%2)").arg(address, pair.first->filter()).replace(QLatin1Char('&'), QLatin1String("&&"));

      QAction* action = menu->addAction(actionText, manager, SLOT(showRule()));
      action->setData(QVariant::fromValue((void*)pair.first));
    }
  }

  menu->addSeparator();

  QVector<WebPage::AdBlockedEntry> entries = page->adBlockedEntries();
  if (entries.isEmpty()) {
    menu->addAction(tr("No content blocked"))->setEnabled(false);
  }
  else {
    menu->addAction(tr("Blocked URL (AdBlock Rule) - click to edit rule"))->setEnabled(false);
    foreach (const WebPage::AdBlockedEntry &entry, entries) {
      QString address = entry.url.toString().right(55);
      QString actionText = tr("%1 with (%2)").arg(address, entry.rule->filter()).replace(QLatin1Char('&'), QLatin1String("&&"));

      QAction* action = menu->addAction(actionText, manager, SLOT(showRule()));
      action->setData(QVariant::fromValue((void*)entry.rule));
    }
  }
}

void AdBlockIcon::showMenu(const QPoint &pos)
{
  QMenu menu;
  createMenu(&menu);

  menu.exec(pos);
}

void AdBlockIcon::toggleCustomFilter()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if (!action) {
    return;
  }

  const QString filter = action->data().toString();
  AdBlockManager* manager = AdBlockManager::instance();
  AdBlockCustomList* customList = manager->customList();

  if (customList->containsFilter(filter)) {
    customList->removeFilter(filter);
  }
  else {
    AdBlockRule* rule = new AdBlockRule(filter, customList);
    customList->addRule(rule);
  }
}

void AdBlockIcon::animateIcon()
{
  ++m_timerTicks;
  if (m_timerTicks > 10) {
    stopAnimation();
    return;
  }

  if (icon().isNull()) {
    setIcon(QIcon(":images/images/adblock.png"));
  }
  else {
    setIcon(QIcon());
  }
}

void AdBlockIcon::stopAnimation()
{
  m_timerTicks = 0;
  m_flashTimer->stop();
  disconnect(m_flashTimer, SIGNAL(timeout()), this, SLOT(animateIcon()));

  setEnabled(m_enabled);
}

void AdBlockIcon::setEnabled(bool enabled)
{
  if (enabled) {
    setIcon(qApp->icons()->fromTheme("web-adblock"));
  }
  else {
    setIcon(qApp->icons()->fromTheme("web-adblock-disabled"));
  }

  m_enabled = enabled;
}

void AdBlockIcon::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
    if (event->modifiers() != Qt::ControlModifier) {
      emit clicked(event->globalPos());
    }
  }
  else {
    QToolButton::mouseReleaseEvent(event);
  }
}

AdBlockIcon::~AdBlockIcon()
{
}
