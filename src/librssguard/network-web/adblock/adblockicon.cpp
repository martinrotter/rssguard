// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockicon.h"

#include "gui/dialogs/formmain.h"
#include "gui/webbrowser.h"
#include "gui/webviewer.h"
#include "miscellaneous/application.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/webpage.h"

#include <QMenu>

AdBlockIcon::AdBlockIcon(AdBlockManager* parent) : QAction(parent), m_manager(parent) {
  setToolTip(tr("AdBlock lets you block unwanted content on web pages"));
  setText(QSL("AdBlock"));
  setMenu(new QMenu());

  connect(m_manager, &AdBlockManager::enabledChanged, this, &AdBlockIcon::setEnabled);
  connect(menu(), &QMenu::aboutToShow, this, [this]() {
    createMenu();
  });
  connect(this, &QAction::triggered, m_manager, &AdBlockManager::showDialog);

  setEnabled(m_manager->isEnabled());
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

  /*
     WebPage* page = qApp->mainForm()->tabWidget()->currentWidget()->webBrowser()->viewer()->page();
     const QUrl page_url = page->url();
     AdBlockCustomList* custom_list = m_manager->customList();

     menu->addSeparator();

     if (!page_url.host().isEmpty() && m_manager->isEnabled() && m_manager->canRunOnScheme(page_url.scheme())) {
     const QString host = page->url().host().contains(QLatin1String("www.")) ? page_url.host().mid(4) : page_url.host();
     const QString host_filter = QString("@@||%1^$document").arg(host);
     const QString page_filter = QString("@@|%1|$document").arg(page_url.toString());
     QAction* act = menu->addAction(tr("Disable on %1").arg(host));

     act->setCheckable(true);
     act->setChecked(custom_list->containsFilter(host_filter));
     act->setData(host_filter);
     connect(act, &QAction::triggered, this, &AdBlockIcon::toggleCustomFilter);

     act = menu->addAction(tr("Disable only on this page"));
     act->setCheckable(true);
     act->setChecked(custom_list->containsFilter(page_filter));
     act->setData(page_filter);
     connect(act, &QAction::triggered, this, &AdBlockIcon::toggleCustomFilter);

     menu->addSeparator();
     }*/
}

void AdBlockIcon::showMenu(const QPoint& pos) {
  QMenu menu;

  createMenu(&menu);
  menu.exec(pos);
}

void AdBlockIcon::setEnabled(bool enabled) {
  if (enabled) {
    setIcon(qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE));
  }
  else {
    setIcon(qApp->icons()->miscIcon(ADBLOCK_ICON_DISABLED));
  }
}
