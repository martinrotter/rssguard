// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockicon.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/adblock/adblockmanager.h"

#include <QMenu>

AdBlockIcon::AdBlockIcon(AdBlockManager* parent) : QAction(parent), m_manager(parent) {
  setToolTip(tr("AdBlock lets you block unwanted content on web pages"));
  setText(QSL("AdBlock"));
  setMenu(new QMenu());

  connect(m_manager, &AdBlockManager::enabledChanged, this, &AdBlockIcon::setIcon);
  connect(m_manager, &AdBlockManager::processTerminated, this, [this]() {
    setIcon(false);
  });

  connect(menu(), &QMenu::aboutToShow, this, [this]() {
    createMenu();
  });
  connect(this, &QAction::triggered, m_manager, &AdBlockManager::showDialog);

  emit m_manager->enabledChanged(m_manager->isEnabled());
}

AdBlockIcon::~AdBlockIcon() {
  if (menu() != nullptr) {
    menu()->deleteLater();
  }
}

void AdBlockIcon::createMenu(QMenu* menu) {
  if (menu == nullptr) {
    menu = qobject_cast<QMenu*>(sender());

    if (menu == nullptr) {
      return;
    }
  }

  menu->clear();
  menu->addAction(tr("Show AdBlock &settings"), m_manager, &AdBlockManager::showDialog);
}

void AdBlockIcon::showMenu(QPoint pos) {
  QMenu menu;

  createMenu(&menu);
  menu.exec(pos);
}

void AdBlockIcon::setIcon(bool adblock_enabled) {
  QAction::setIcon(adblock_enabled
                   ? qApp->icons()->miscIcon(QSL(ADBLOCK_ICON_ACTIVE))
                   : qApp->icons()->miscIcon(QSL(ADBLOCK_ICON_DISABLED)));
}
