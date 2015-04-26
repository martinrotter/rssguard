#include "network-web/discoverfeedsbutton.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"


DiscoverFeedsButton::DiscoverFeedsButton(QWidget *parent) : QToolButton(parent) {
  setEnabled(false);
  setIcon(qApp->icons()->fromTheme("folder-feed"));
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
    setMenu(new QMenu(this));

    // TODO: pokračovat asi zde, po kliku na menu vyslat signal, ten odchytne webbrowser a provede akce.
  }

  menu()->hide();

  if (!addresses.isEmpty()) {
    menu()->clear();

    foreach (const QString &feed, addresses) {
      menu()->addAction(feed);
    }

    connect(this, SIGNAL(clicked(bool)), this, SLOT(showMenu()));
  }
}
