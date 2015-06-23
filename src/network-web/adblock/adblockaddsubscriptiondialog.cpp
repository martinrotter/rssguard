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

#include "network-web/adblock/adblockaddsubscriptiondialog.h"

#include "network-web/adblock/adblockmanager.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "gui/lineeditwithstatus.h"

#include <QPushButton>


AdBlockAddSubscriptionDialog::AdBlockAddSubscriptionDialog(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::AdBlockAddSubscriptionDialog) {
  m_ui->setupUi(this);

  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);
  setWindowIcon(qApp->icons()->fromTheme("web-adblock"));

  setTabOrder(m_ui->m_cmbPresets, m_ui->m_txtTitle->lineEdit());
  setTabOrder(m_ui->m_txtTitle->lineEdit(), m_ui->m_txtUrl->lineEdit());
  setTabOrder(m_ui->m_txtUrl->lineEdit(), m_ui->m_buttonBox);

  m_knownSubscriptions << Subscription("EasyList (English)", ADBLOCK_EASYLIST_URL)
                       << Subscription("Fanboy's List (English)", "http://www.fanboy.co.nz/adblock/fanboy-adblock.txt")
                       << Subscription("Adversity (English)", "http://adversity.googlecode.com/hg/Adversity.txt")
                       << Subscription("BSI Lista Polska (Polish)", "http://www.bsi.info.pl/filtrABP.txt")
                       << Subscription("Czech List (Czech)", "http://adblock.dajbych.net/adblock.txt")
                       << Subscription("dutchblock (Dutch)", "http://groenewoudt.net/dutchblock/list.txt")
                       << Subscription("Filtros Nauscopicos (Spanish)", "http://abp.mozilla-hispano.org/nauscopio/filtros.txt")
                       << Subscription("hufilter (Hungarian)", "http://www.hufilter.hu/hufilter.txt")
                       << Subscription("IsraelList (Hebrew)", "http://secure.fanboy.co.nz/israelilist/IsraelList.txt")
                       << Subscription("Lista Basa (Polish)", "http://plok.studentlive.pl/abp.txt")
                       << Subscription("NLBlock (Dutch)", "http://www.verzijlbergh.com/adblock/nlblock.txt")
                       << Subscription("Peter Lowe's list (English)", "http://pgl.yoyo.org/adservers/serverlist.php?hostformat=adblockplus&mimetype=plaintext")
                       << Subscription("PLgeneral (Polish)", "http://www.niecko.pl/adblock/adblock.txt")
                       << Subscription("Schacks Adblock Plus liste (Danish)", "http://adblock.schack.dk/block.txt")
                       << Subscription("Xfiles (Italian)", "http://mozilla.gfsolone.com/filtri.txt")
                       << Subscription("EasyPrivacy (English)", "http://easylist-downloads.adblockplus.org/easyprivacy.txt")
                       << Subscription("Antisocial (English)", "http://adversity.googlecode.com/hg/Antisocial.txt")
                       << Subscription("RuAdList+EasyList (Russian, Ukrainian)", "https://easylist-downloads.adblockplus.org/ruadlist+easylist.txt")
                       << Subscription("RU AdList (Russian, Ukrainian)", "https://easylist-downloads.adblockplus.org/advblock.txt")
                       << Subscription("ABPindo (Indonesian)", "https://indonesianadblockrules.googlecode.com/hg/subscriptions/abpindo.txt")
                       << Subscription("ChinaList (Chinese)", "http://adblock-chinalist.googlecode.com/svn/trunk/adblock.txt")
                       << Subscription("Malware Domains list", "https://easylist-downloads.adblockplus.org/malwaredomains_full.txt") <<
                          Subscription(tr("Another subscription"), QString());

  foreach (const Subscription &subscription, m_knownSubscriptions) {
    m_ui->m_cmbPresets->addItem(subscription.m_title);
  }

  connect(m_ui->m_cmbPresets, SIGNAL(currentIndexChanged(int)), this, SLOT(onSubscriptionPresetChanged(int)));
  connect(m_ui->m_txtTitle->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(checkInputs()));
  connect(m_ui->m_txtUrl->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(checkInputs()));
  onSubscriptionPresetChanged(0);
}

QString AdBlockAddSubscriptionDialog::title() const {
  return m_ui->m_txtTitle->lineEdit()->text();
}

QString AdBlockAddSubscriptionDialog::url() const {
  return m_ui->m_txtUrl->lineEdit()->text();
}

void AdBlockAddSubscriptionDialog::onSubscriptionPresetChanged(int index) {
  const Subscription subscription = m_knownSubscriptions.at(index);

  // "Other" entry.
  if (subscription.m_url.isEmpty()) {
    m_ui->m_txtTitle->lineEdit()->clear();
    m_ui->m_txtUrl->lineEdit()->clear();
  }
  else {
    m_ui->m_txtTitle->lineEdit()->setText( subscription.m_title);
    m_ui->m_txtTitle->lineEdit()->setCursorPosition(0);
    m_ui->m_txtUrl->lineEdit()->setText(subscription.m_url);
    m_ui->m_txtUrl->lineEdit()->setCursorPosition(0);
  }
}

void AdBlockAddSubscriptionDialog::checkInputs() {
  bool is_ok = true;

  if (!m_ui->m_txtTitle->lineEdit()->text().simplified().isEmpty()) {
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::Ok, tr("Entered title is okay."));
  }
  else {
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::Error, tr("Entered title is empty."));
    is_ok = false;
  }

  if (!m_ui->m_txtUrl->lineEdit()->text().simplified().isEmpty()) {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::Ok, tr("Entered url is okay."));
  }
  else {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::Error, tr("Entered url is empty."));
    is_ok = false;
  }

  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(is_ok);
}

AdBlockAddSubscriptionDialog::~AdBlockAddSubscriptionDialog() {
  delete m_ui;
}
