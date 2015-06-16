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
#include "adblockaddsubscriptiondialog.h"
#include "adblockmanager.h"
#include "ui_adblockaddsubscriptiondialog.h"

AdBlockAddSubscriptionDialog::AdBlockAddSubscriptionDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::AdBlockAddSubscriptionDialog)
{
  ui->setupUi(this);

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
                       << Subscription("Malware Domains list", "https://easylist-downloads.adblockplus.org/malwaredomains_full.txt");

  foreach (const Subscription &subscription, m_knownSubscriptions) {
    ui->comboBox->addItem(subscription.title);
  }

  connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(indexChanged(int)));
  indexChanged(0);
}

QString AdBlockAddSubscriptionDialog::title() const
{
  return ui->title->text();
}

QString AdBlockAddSubscriptionDialog::url() const
{
  return ui->url->text();
}

void AdBlockAddSubscriptionDialog::indexChanged(int index)
{
  const Subscription subscription = m_knownSubscriptions.at(index);

  // "Other..." entry
  if (subscription.url.isEmpty()) {
    ui->title->clear();
    ui->url->clear();
  }
  else {
    int pos = subscription.title.indexOf(QLatin1Char('('));
    QString title = subscription.title;

    if (pos > 0) {
      title = title.left(pos).trimmed();
    }

    ui->title->setText(title);
    ui->title->setCursorPosition(0);

    ui->url->setText(subscription.url);
    ui->url->setCursorPosition(0);
  }
}

AdBlockAddSubscriptionDialog::~AdBlockAddSubscriptionDialog()
{
  delete ui;
}
