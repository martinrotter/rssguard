// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/discoverfeedsbutton.h"

#include "core/feedsmodel.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

#include <QVariant>

DiscoverFeedsButton::DiscoverFeedsButton(QWidget* parent) : QToolButton(parent), m_addresses(QStringList()) {
  setEnabled(false);
  setIcon(qApp->icons()->fromTheme(QSL("application-rss+xml")));
  setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
}

DiscoverFeedsButton::~DiscoverFeedsButton() {}

void DiscoverFeedsButton::clearFeedAddresses() {
  setFeedAddresses({});
}

void DiscoverFeedsButton::setFeedAddresses(const QStringList& addresses) {
  setEnabled(!addresses.isEmpty());
  setToolTip(addresses.isEmpty() ?
             tr("This website does not contain any feeds") :
             tr("Add one of %n feed(s)", 0, addresses.size()));

  if (menu() == nullptr) {
    // Initialize the menu.
    setMenu(new QMenu(this));
    connect(menu(), &QMenu::triggered, this, &DiscoverFeedsButton::linkTriggered);
    connect(menu(), &QMenu::aboutToShow, this, &DiscoverFeedsButton::fillMenu);
  }

  menu()->hide();
  m_addresses = addresses;
}

void DiscoverFeedsButton::linkTriggered(QAction* action) {
  const QString url = action->property("url").toString();
  ServiceRoot* root = static_cast<ServiceRoot*>(action->property("root").value<void*>());

  if (root->supportsFeedAdding()) {
    root->addNewFeed(qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsView()->selectedItem(), url);
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent, {
      tr("Not supported by account"),
      tr("Given account does not support adding feeds."),
      QSystemTrayIcon::MessageIcon::Warning });
  }
}

void DiscoverFeedsButton::fillMenu() {
  menu()->clear();
  auto srts = qApp->feedReader()->feedsModel()->serviceRoots();

  for (const ServiceRoot* root : qAsConst(srts)) {
    if (root->supportsFeedAdding()) {
      QMenu* root_menu = menu()->addMenu(root->icon(), root->title());

      for (const QString& url : qAsConst(m_addresses)) {
        QAction* url_action = root_menu->addAction(root->icon(), url);

        url_action->setProperty("url", url);
        url_action->setProperty("root", QVariant::fromValue((void*) root));
      }
    }
  }

  if (menu()->isEmpty()) {
    menu()->addAction(tr("Feeds were detected, but no suitable accounts are configured."))->setEnabled(false);
  }
}
