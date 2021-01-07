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

/**
 * Copyright (c) 2009, Benjamin C. Meyer <ben@meyerhome.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "network-web/adblock/adblocksubscription.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblocksearchtree.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"

#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QSaveFile>
#include <QTimer>
#include <utility>

AdBlockSubscription::AdBlockSubscription(QString title, QObject* parent)
  : QObject(parent), m_reply(nullptr), m_title(std::move(title)), m_updated(false) {}

QString AdBlockSubscription::title() const {
  return m_title;
}

QString AdBlockSubscription::filePath() const {
  return m_filePath;
}

void AdBlockSubscription::setFilePath(const QString& path) {
  m_filePath = path;
}

QUrl AdBlockSubscription::url() const {
  return m_url;
}

void AdBlockSubscription::setUrl(const QUrl& url) {
  m_url = url;
}

void AdBlockSubscription::loadSubscription(const QStringList& disabledRules) {
  QFile file(m_filePath);

  if (!file.exists()) {
    QTimer::singleShot(0, this, &AdBlockSubscription::updateSubscription);
    return;
  }

  if (!file.open(QFile::ReadOnly)) {
    qWarningNN << LOGSEC_ADBLOCK
               << "Unable to open adblock file"
               << QUOTE_W_SPACE(m_filePath)
               << "for reading.";
    QTimer::singleShot(0, this, &AdBlockSubscription::updateSubscription);
    return;
  }

  QTextStream textStream(&file);

  textStream.setCodec("UTF-8");

  // Header is on 3rd line.
  textStream.readLine(1024);
  textStream.readLine(1024);
  QString header = textStream.readLine(1024);

  if (!header.startsWith(QL1S("[Adblock")) || m_title.isEmpty()) {
    qWarningNN << LOGSEC_ADBLOCK
               << "Invalid format of AdBlock file"
               << QUOTE_W_SPACE_DOT(m_filePath);
    QTimer::singleShot(0, this, &AdBlockSubscription::updateSubscription);
    return;
  }

  m_rules.clear();

  while (!textStream.atEnd()) {
    AdBlockRule* rule = new AdBlockRule(textStream.readLine(), this);

    if (disabledRules.contains(rule->filter())) {
      rule->setEnabled(false);
    }

    m_rules.append(rule);
  }

  // Initial update.
  if (m_rules.isEmpty() && !m_updated) {
    QTimer::singleShot(0, this, &AdBlockSubscription::updateSubscription);
  }
}

void AdBlockSubscription::saveSubscription() {}

void AdBlockSubscription::updateSubscription() {
  if ((m_reply != nullptr) || !m_url.isValid()) {
    return;
  }

  auto* mgs = new SilentNetworkAccessManager(this);

  m_reply = mgs->get(QNetworkRequest(m_url));
  connect(m_reply, &QNetworkReply::finished, this, &AdBlockSubscription::subscriptionDownloaded);
}

void AdBlockSubscription::subscriptionDownloaded() {
  if (m_reply != qobject_cast<QNetworkReply*>(sender())) {
    return;
  }

  bool error = false;
  const QByteArray response = QString::fromUtf8(m_reply->readAll()).toUtf8();

  if (m_reply->error() != QNetworkReply::NoError || !response.startsWith(QByteArray("[Adblock")) || !saveDownloadedData(response)) {
    error = true;
  }

  m_reply->manager()->deleteLater();
  m_reply->deleteLater();
  m_reply = nullptr;

  if (error) {
    emit subscriptionError(tr("Cannot load subscription!"));

    return;
  }

  loadSubscription(qApp->web()->adBlock()->disabledRules());
  emit subscriptionUpdated();
  emit subscriptionChanged();
}

bool AdBlockSubscription::saveDownloadedData(const QByteArray& data) {
  QSaveFile file(m_filePath);

  if (!file.open(QFile::WriteOnly)) {
    qWarningNN << LOGSEC_ADBLOCK
               << "Unable to open AdBlock file"
               << QUOTE_W_SPACE(m_filePath)
               << "for writing.";
    return false;
  }
  else {
    // Write subscription header
    file.write(QString("Title: %1\nUrl: %2\n").arg(title(), url().toString()).toUtf8());
    file.write(data);
    file.commit();
    return true;
  }
}

const AdBlockRule* AdBlockSubscription::rule(int offset) const {
  if (IS_IN_ARRAY(offset, m_rules)) {
    return m_rules[offset];
  }
  else {
    return nullptr;
  }
}

QVector<AdBlockRule*> AdBlockSubscription::allRules() const {
  return m_rules;
}

const AdBlockRule* AdBlockSubscription::enableRule(int offset) {
  if (IS_IN_ARRAY(offset, m_rules)) {
    AdBlockRule* rule = m_rules[offset];

    rule->setEnabled(true);
    qApp->web()->adBlock()->removeDisabledRule(rule->filter());
    emit subscriptionChanged();

    return rule;
  }
  else {
    return nullptr;
  }
}

const AdBlockRule* AdBlockSubscription::disableRule(int offset) {
  if (!IS_IN_ARRAY(offset, m_rules)) {
    return nullptr;
  }

  AdBlockRule* rule = m_rules[offset];

  rule->setEnabled(false);
  qApp->web()->adBlock()->addDisabledRule(rule->filter());
  emit subscriptionChanged();

  return rule;
}

bool AdBlockSubscription::canEditRules() const {
  return false;
}

bool AdBlockSubscription::canBeRemoved() const {
  return true;
}

int AdBlockSubscription::addRule(AdBlockRule* rule) {
  Q_UNUSED(rule)
  return -1;
}

bool AdBlockSubscription::removeRule(int offset) {
  Q_UNUSED(offset)
  return false;
}

const AdBlockRule* AdBlockSubscription::replaceRule(AdBlockRule* rule, int offset) {
  Q_UNUSED(rule)
  Q_UNUSED(offset)
  return nullptr;
}

AdBlockSubscription::~AdBlockSubscription() {
  qDeleteAll(m_rules);
}

// AdBlockCustomList

AdBlockCustomList::AdBlockCustomList(QObject* parent)
  : AdBlockSubscription(tr("Custom rules"), parent) {
  setFilePath(AdBlockManager::storedListsPath() + QDir::separator() + ADBLOCK_CUSTOMLIST_NAME);
}

void AdBlockCustomList::loadSubscription(const QStringList& disabledRules) {
  // DuckDuckGo ad whitelist rules
  // They cannot be removed, but can be disabled.
  // Please consider not disabling them. Thanks!
  const QString ddg1 = QSL("@@||duckduckgo.com^$document");
  const QString ddg2 = QSL("duckduckgo.com#@#.has-ad");
  QString rules;

  try {
    rules = QString::fromUtf8(IOFactory::readFile(filePath()));
  }
  catch (ApplicationException&) {}

  QFile file(filePath());

  if (!file.exists()) {
    saveSubscription();
  }

  if (file.open(QFile::WriteOnly | QFile::Append)) {
    QTextStream stream(&file);

    stream.setCodec("UTF-8");

    if (!rules.contains(ddg1 + QL1C('\n'))) {
      stream << ddg1 << QL1C('\n');
    }

    if (!rules.contains(QL1C('\n') + ddg2)) {
      stream << ddg2 << QL1C('\n');
    }
  }

  file.close();
  AdBlockSubscription::loadSubscription(disabledRules);
}

void AdBlockCustomList::saveSubscription() {
  QFile file(filePath());

  if (!file.open(QFile::ReadWrite | QFile::Truncate)) {
    qWarningNN << LOGSEC_ADBLOCK
               << "Unable to open AdBlock file"
               << QUOTE_W_SPACE(filePath())
               << "for writing.";
    return;
  }

  QTextStream textStream(&file);

  textStream.setCodec("UTF-8");
  textStream << "Title: " << title() << QL1C('\n');
  textStream << "Url: " << url().toString() << QL1C('\n');
  textStream << "[Adblock Plus 1.1.1]" << QL1C('\n');

  for (const AdBlockRule* rule : m_rules) {
    textStream << rule->filter() << QL1C('\n');
  }

  file.close();
}

bool AdBlockCustomList::canEditRules() const {
  return true;
}

bool AdBlockCustomList::canBeRemoved() const {
  return false;
}

bool AdBlockCustomList::containsFilter(const QString& filter) const {
  for (const AdBlockRule* rule : m_rules) {
    if (rule->filter() == filter) {
      return true;
    }
  }

  return false;
}

bool AdBlockCustomList::removeFilter(const QString& filter) {
  for (int i = 0; i < m_rules.count(); ++i) {
    const AdBlockRule* rule = m_rules.at(i);

    if (rule->filter() == filter) {
      return removeRule(i);
    }
  }

  return false;
}

int AdBlockCustomList::addRule(AdBlockRule* rule) {
  m_rules.append(rule);
  emit subscriptionChanged();

  return m_rules.count() - 1;
}

bool AdBlockCustomList::removeRule(int offset) {
  if (!IS_IN_ARRAY(offset, m_rules)) {
    return false;
  }

  AdBlockRule* rule = m_rules.at(offset);
  const QString filter = rule->filter();

  m_rules.remove(offset);
  emit subscriptionChanged();

  qApp->web()->adBlock()->removeDisabledRule(filter);
  delete rule;
  return true;
}

const AdBlockRule* AdBlockCustomList::replaceRule(AdBlockRule* rule, int offset) {
  if (!IS_IN_ARRAY(offset, m_rules)) {
    return nullptr;
  }

  AdBlockRule* oldRule = m_rules.at(offset);

  m_rules[offset] = rule;
  emit subscriptionChanged();

  delete oldRule;
  return m_rules[offset];
}
