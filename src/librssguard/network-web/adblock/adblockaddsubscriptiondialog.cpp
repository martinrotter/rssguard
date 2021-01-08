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

#include "network-web/adblock/adblockaddsubscriptiondialog.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

AdBlockAddSubscriptionDialog::AdBlockAddSubscriptionDialog(QWidget* parent)
  : QDialog(parent), m_ui(new Ui::AdBlockAddSubscriptionDialog) {
  m_ui->setupUi(this);
  m_knownSubscriptions
    << Subscription(QSL("EasyList"),
                    QSL(ADBLOCK_EASYLIST_URL))
    << Subscription(QSL("EasyPrivacy"),
                  QSL("https://easylist.to/easylist/easyprivacy.txt"))
    << Subscription(QSL("EasyPrivacy Tracking Protection List"),
                  QSL("https://easylist-downloads.adblockplus.org/easyprivacy.tpl"))
    << Subscription(QSL("Adblock Warning Removal List"),
                  QSL("https://easylist-downloads.adblockplus.org/antiadblockfilters.txt"));

  for (const Subscription& subscription : m_knownSubscriptions) {
    m_ui->m_cmbPresets->addItem(subscription.m_title);
  }

  connect(m_ui->m_cmbPresets, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &AdBlockAddSubscriptionDialog::indexChanged);
  connect(m_ui->m_cbUsePredefined, &QCheckBox::toggled, this,
          &AdBlockAddSubscriptionDialog::presetsEnabledChanged);

  m_ui->m_cbUsePredefined->setChecked(true);
  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE),
                                      tr("Add subscription"));
}

QString AdBlockAddSubscriptionDialog::title() const {
  return m_ui->m_txtTitle->text();
}

QString AdBlockAddSubscriptionDialog::url() const {
  return m_ui->m_txtUrl->text();
}

void AdBlockAddSubscriptionDialog::indexChanged(int index) {
  const Subscription subscription = m_knownSubscriptions.at(index);
  const int pos = subscription.m_title.indexOf(QLatin1Char('('));

  if (pos > 0) {
    m_ui->m_txtTitle->setText(subscription.m_title.left(pos).trimmed());
  }
  else {
    m_ui->m_txtTitle->setText(subscription.m_title);
  }

  m_ui->m_txtUrl->setText(subscription.m_url);
}

void AdBlockAddSubscriptionDialog::presetsEnabledChanged(bool enabled) {
  m_ui->m_txtTitle->setEnabled(!enabled);
  m_ui->m_txtUrl->setEnabled(!enabled);
  m_ui->m_cmbPresets->setEnabled(enabled);

  if (!enabled) {
    m_ui->m_txtTitle->clear();
    m_ui->m_txtUrl->clear();
    m_ui->m_txtTitle->setFocus();
  }
  else {
    indexChanged(m_ui->m_cmbPresets->currentIndex());
  }
}

AdBlockAddSubscriptionDialog::~AdBlockAddSubscriptionDialog() {
  delete m_ui;
}

AdBlockAddSubscriptionDialog::Subscription::Subscription() {}

AdBlockAddSubscriptionDialog::Subscription::Subscription(const QString& title, const QString& url) {
  m_title = title;
  m_url = url;
}
