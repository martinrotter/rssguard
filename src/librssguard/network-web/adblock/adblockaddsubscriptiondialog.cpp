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
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCheckBox>
#include <QComboBox>

AdBlockAddSubscriptionDialog::AdBlockAddSubscriptionDialog(QWidget* parent)
  : QDialog(parent), m_ui(new Ui::AdBlockAddSubscriptionDialog) {
  m_ui->setupUi(this);
  m_knownSubscriptions << Subscription(QSL("EasyList (English)"), ADBLOCK_EASYLIST_URL)
                       << Subscription(QSL("BSI Lista Polska (Polish)"), QSL("http://www.bsi.info.pl/filtrABP.txt"))
                       << Subscription(QSL("EasyList Germany (German"), QSL("https://easylist.to/easylistgermany/easylistgermany.txt"))
                       << Subscription(QSL("EasyList Czech and Slovak (Czech)"),
                  QSL("https://raw.githubusercontent.com/tomasko126/easylistczechandslovak/master/filters.txt"))
                       << Subscription(QSL("dutchblock (Dutch)"), QSL("http://groenewoudt.net/dutchblock/list.txt"))
                       << Subscription(QSL("Filtros Nauscopicos (Spanish)"), QSL("http://abp.mozilla-hispano.org/nauscopio/filtros.txt"))
                       << Subscription(QSL("IsraelList (Hebrew)"), QSL("http://secure.fanboy.co.nz/israelilist/IsraelList.txt"))
                       << Subscription(QSL("NLBlock (Dutch)"), QSL("http://www.verzijlbergh.com/adblock/nlblock.txt"))
                       << Subscription(QSL("Peter Lowe's list (English)"),
                  QSL("http://pgl.yoyo.org/adservers/serverlist.php?hostformat=adblockplus&mimetype=plaintext"))
                       << Subscription(QSL("PLgeneral (Polish)"), QSL("http://www.niecko.pl/adblock/adblock.txt"))
                       << Subscription(QSL("Schacks Adblock Plus liste (Danish)"), QSL("http://adblock.schack.dk/block.txt"))
                       << Subscription(QSL("Xfiles (Italian)"), QSL("http://mozilla.gfsolone.com/filtri.txt"))
                       << Subscription(QSL("EasyPrivacy (English)"), QSL("http://easylist-downloads.adblockplus.org/easyprivacy.txt"))
                       << Subscription(QSL("RU Adlist (Russian)"), QSL("https://easylist-downloads.adblockplus.org/advblock.txt"))
                       << Subscription(QSL("ABPindo (Indonesian)"),
                  QSL("https://raw.githubusercontent.com/heradhis/indonesianadblockrules/master/subscriptions/abpindo.txt"))
                       << Subscription(QSL("Easylist China (Chinese)"), QSL("https://easylist-downloads.adblockplus.org/easylistchina.txt"))
                       << Subscription(QSL("Anti-Adblock Killer"),
                  QSL("https://raw.githubusercontent.com/reek/anti-adblock-killer/master/anti-adblock-killer-filters.txt"));

  for (const Subscription& subscription : m_knownSubscriptions) {
    m_ui->m_cmbPresets->addItem(subscription.m_title);
  }

  connect(m_ui->m_cmbPresets, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &AdBlockAddSubscriptionDialog::indexChanged);
  connect(m_ui->m_cbUsePredefined, &QCheckBox::toggled, this, &AdBlockAddSubscriptionDialog::presetsEnabledChanged);
  m_ui->m_cbUsePredefined->setChecked(true);
  indexChanged(0);
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);
  setWindowIcon(qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE));
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

  if (!enabled) {
    // Presets are disabled, clear txts.
    m_ui->m_txtTitle->clear();
    m_ui->m_txtUrl->clear();
    m_ui->m_txtTitle->setFocus();
  }
}

AdBlockAddSubscriptionDialog::~AdBlockAddSubscriptionDialog() {
  delete m_ui;
}
