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

#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblocksearchtree.h"
#include "network-web/downloader.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/application.h"
#include "exceptions/applicationexception.h"

#include <QFile>
#include <QTimer>
#include <QNetworkReply>
#include <QDir>
#include <QWebEnginePage>


AdBlockSubscription::AdBlockSubscription(const QString &title, QObject *parent)
  : QObject(parent), m_title(title), m_downloadingSubscription(false), m_updated(false) {
}

QString AdBlockSubscription::title() const {
  return m_title;
}

void AdBlockSubscription::setTitle(const QString &title) {
  m_title = title;
}

QString AdBlockSubscription::filePath() const {
  return m_filePath;
}

void AdBlockSubscription::setFilePath(const QString &path) {
  m_filePath = path;
}

QUrl AdBlockSubscription::url() const {
  return m_url;
}

void AdBlockSubscription::setUrl(const QUrl &url) {
  m_url = url;
}

void AdBlockSubscription::loadSubscription(const QStringList &disabled_rules) {
  QFile file(m_filePath);

  if (!file.exists()) {
    qWarning("Cannot load subscription '%s'. Requesting its update.", qPrintable(title()));
    QTimer::singleShot(0, this, SLOT(updateSubscription()));
    return;
  }

  if (!file.open(QFile::ReadOnly)) {
    qWarning("Unable to open subscription file '%s' for reading.", qPrintable(QDir::toNativeSeparators(m_filePath)));
    QTimer::singleShot(0, this, SLOT(updateSubscription()));
    return;
  }

  QTextStream textStream(&file);
  textStream.setCodec("UTF-8");

  // Header is on 3rd line.
  textStream.readLine(1024);
  textStream.readLine(1024);
  QString header = textStream.readLine(1024);

  if (!header.startsWith(QL1S("[Adblock")) || m_title.isEmpty()) {
    qWarning("Invalid format of subscription file '%s'.", qPrintable(QDir::toNativeSeparators(m_filePath)));
    file.close();
    QTimer::singleShot(0, this, SLOT(updateSubscription()));
    return;
  }

  m_rules.clear();

  while (!textStream.atEnd()) {
    AdBlockRule *rule = new AdBlockRule(textStream.readLine(), this);

    if (disabled_rules.contains(rule->filter())) {
      rule->setEnabled(false);
    }

    m_rules.append(rule);
  }

  file.close();

  // Initial update.
  if (m_rules.isEmpty() && !m_updated) {
    QTimer::singleShot(0, this, SLOT(updateSubscription()));
  }
}

void AdBlockSubscription::saveSubscription() {
}

void AdBlockSubscription::updateSubscription() {
  if (m_downloadingSubscription || !m_url.isValid()) {
    return;
  }

  m_downloadingSubscription = true;

  Downloader *downloader = new Downloader();

  connect(downloader, SIGNAL(completed(QNetworkReply::NetworkError,QByteArray)), this, SLOT(subscriptionDownloaded()));
  downloader->downloadFile(m_url.toString());
}

void AdBlockSubscription::subscriptionDownloaded() {
  Downloader *downloader = qobject_cast<Downloader*>(sender());

  if (downloader == NULL) {
    return;
  }

  bool error = false;
  QByteArray response = QString::fromUtf8(downloader->lastOutputData()).toUtf8();

  if (response.startsWith(' ')) {
    // Deal with " [Adblock".
    response = response.remove(0, 1);
  }

  if (downloader->lastOutputError() != QNetworkReply::NoError ||
      !response.startsWith(QByteArray("[Adblock")) ||
      !saveDownloadedData(response)) {
    error = true;
  }

  downloader->deleteLater();

  if (error) {
    emit subscriptionError(tr("Cannot load subscription!"));
  }
  else {
    loadSubscription(AdBlockManager::instance()->disabledRules());

    emit subscriptionUpdated();
    emit subscriptionChanged();
  }

  m_downloadingSubscription = false;
}

bool AdBlockSubscription::saveDownloadedData(const QByteArray &data) {
  QFile file(m_filePath);

  if (!file.open(QFile::ReadWrite | QFile::Truncate)) {
    qWarning("Unable to open subscription file '%s' for writting.", qPrintable(QDir::toNativeSeparators(m_filePath)));
    return false;
  }

  // Write subscription header
  file.write(QString("Title: %1\nUrl: %2\n").arg(title(), url().toString()).toUtf8());

  if (AdBlockManager::instance()->useLimitedEasyList() && m_url == QUrl(ADBLOCK_EASYLIST_URL)) {
    // Third-party advertisers rules are with start domain (||) placeholder which needs regexps
    // So we are ignoring it for keeping good performance
    // But we will use whitelist rules at the end of list

    QByteArray part1 = data.left(data.indexOf(QL1S("!-----------------------------Third-party adverts-----------------------------!")));
    QByteArray part2 = data.mid(data.indexOf(QL1S("!---------------------------------Whitelists----------------------------------!")));

    file.write(part1);
    file.write(part2);
  }
  else {
    file.write(data);
  }

  file.flush();
  file.close();

  return true;
}

const AdBlockRule *AdBlockSubscription::rule(int offset) const{
  if (offset >= 0 && m_rules.size() > offset) {
    return m_rules[offset];

  }
  else {
    return NULL;
  }
}

QVector<AdBlockRule*> AdBlockSubscription::allRules() const {
  return m_rules;
}

const AdBlockRule *AdBlockSubscription::enableRule(int offset) {
  if (offset >= 0 && m_rules.size() > offset) {
    AdBlockRule *rule = m_rules[offset];
    rule->setEnabled(true);
    AdBlockManager::instance()->removeDisabledRule(rule->filter());

    emit subscriptionChanged();

    return rule;
  }
  else {
    return NULL;
  }
}

const AdBlockRule *AdBlockSubscription::disableRule(int offset) {
  if (offset >= 0 && m_rules.size() > offset) {
    AdBlockRule *rule = m_rules[offset];
    rule->setEnabled(false);
    AdBlockManager::instance()->addDisabledRule(rule->filter());

    emit subscriptionChanged();

    return rule;
  }
  else {
    return NULL;
  }
}

bool AdBlockSubscription::canEditRules() const {
  return false;
}

bool AdBlockSubscription::canBeRemoved() const {
  return true;
}

int AdBlockSubscription::addRule(AdBlockRule *rule) {
  Q_UNUSED(rule)
  return -1;
}

bool AdBlockSubscription::removeRule(int offset) {
  Q_UNUSED(offset)
  return false;
}

const AdBlockRule *AdBlockSubscription::replaceRule(AdBlockRule *rule, int offset) {
  Q_UNUSED(rule)
  Q_UNUSED(offset)
  return 0;
}

AdBlockSubscription::~AdBlockSubscription() {
  qDeleteAll(m_rules);
}

AdBlockCustomList::AdBlockCustomList(QObject *parent) : AdBlockSubscription(tr("Custom rules"), parent) {
  setTitle(tr("Custom rules"));
  setFilePath(AdBlockManager::baseSubscriptionDirectory() + QDir::separator() + ADBLOCK_CUSTOM_LIST_FILENAME);
}

AdBlockCustomList::~AdBlockCustomList() {
}

void AdBlockCustomList::saveSubscription() {
  QFile file(m_filePath);

  if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
    qWarning("Unable to open custom subscription file '%s' for writting.", qPrintable(QDir::toNativeSeparators(m_filePath)));
    return;
  }

  QTextStream textStream(&file);
  textStream.setCodec("UTF-8");
  textStream << "Title: " << title() << endl;
  textStream << "Url: " << url().toString() << endl;
  textStream << "[Adblock Plus 1.1.1]" << endl;

  foreach (const AdBlockRule *rule, m_rules) {
    textStream << rule->filter() << endl;
  }

  file.flush();
  file.close();
}

bool AdBlockCustomList::canEditRules() const {
  return true;
}

bool AdBlockCustomList::canBeRemoved() const {
  return false;
}

bool AdBlockCustomList::containsFilter(const QString &filter) const {
  foreach (const AdBlockRule *rule, m_rules) {
    if (rule->filter() == filter) {
      return true;
    }
  }

  return false;
}

bool AdBlockCustomList::removeFilter(const QString &filter) {
  for (int i = 0; i < m_rules.size(); i++) {
    const AdBlockRule *rule = m_rules.at(i);

    if (rule->filter() == filter) {
      return removeRule(i);
    }
  }

  return false;
}

int AdBlockCustomList::addRule(AdBlockRule *rule) {
  m_rules.append(rule);
  emit subscriptionChanged();

  return m_rules.size() - 1;
}

bool AdBlockCustomList::removeRule(int offset) {
  if (offset >= 0 && m_rules.size() > offset) {
    AdBlockRule *rule = m_rules.at(offset);
    const QString filter = rule->filter();

    m_rules.remove(offset);
    emit subscriptionChanged();

    AdBlockManager::instance()->removeDisabledRule(filter);
    delete rule;
    return true;
  }
  else {
    return false;
  }
}

const AdBlockRule *AdBlockCustomList::replaceRule(AdBlockRule *rule, int offset) {
  if (offset >= 0 && m_rules.size() > offset) {
    AdBlockRule *oldRule = m_rules.at(offset);
    m_rules[offset] = rule;
    emit subscriptionChanged();

    delete oldRule;
    return m_rules[offset];
  }
  else {
    return NULL;
  }
}
