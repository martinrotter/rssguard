/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2010-2017 David Rosca <nowrep@gmail.com>
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
#include "adblocksubscription.h"
#include "adblockmanager.h"
#include "adblocksearchtree.h"
#include "mainapplication.h"
#include "networkmanager.h"
#include "datapaths.h"
#include "qztools.h"

#include <QFile>
#include <QTimer>
#include <QNetworkReply>
#include <QSaveFile>

AdBlockSubscription::AdBlockSubscription(const QString &title, QObject* parent)
    : QObject(parent)
    , m_reply(0)
    , m_title(title)
    , m_updated(false)
{
}

QString AdBlockSubscription::title() const
{
    return m_title;
}

QString AdBlockSubscription::filePath() const
{
    return m_filePath;
}

void AdBlockSubscription::setFilePath(const QString &path)
{
    m_filePath = path;
}

QUrl AdBlockSubscription::url() const
{
    return m_url;
}

void AdBlockSubscription::setUrl(const QUrl &url)
{
    m_url = url;
}

void AdBlockSubscription::loadSubscription(const QStringList &disabledRules)
{
    QFile file(m_filePath);

    if (!file.exists()) {
        QTimer::singleShot(0, this, SLOT(updateSubscription()));
        return;
    }

    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "AdBlockSubscription::" << __FUNCTION__ << "Unable to open adblock file for reading" << m_filePath;
        QTimer::singleShot(0, this, SLOT(updateSubscription()));
        return;
    }

    QTextStream textStream(&file);
    textStream.setCodec("UTF-8");
    // Header is on 3rd line
    textStream.readLine(1024);
    textStream.readLine(1024);
    QString header = textStream.readLine(1024);

    if (!header.startsWith(QLatin1String("[Adblock")) || m_title.isEmpty()) {
        qWarning() << "AdBlockSubscription::" << __FUNCTION__ << "invalid format of adblock file" << m_filePath;
        QTimer::singleShot(0, this, SLOT(updateSubscription()));
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

    // Initial update
    if (m_rules.isEmpty() && !m_updated) {
        QTimer::singleShot(0, this, SLOT(updateSubscription()));
    }
}

void AdBlockSubscription::saveSubscription()
{
}

void AdBlockSubscription::updateSubscription()
{
    if (m_reply || !m_url.isValid()) {
        return;
    }

    m_reply = mApp->networkManager()->get(QNetworkRequest(m_url));
    connect(m_reply, &QNetworkReply::finished, this, &AdBlockSubscription::subscriptionDownloaded);
}

void AdBlockSubscription::subscriptionDownloaded()
{
    if (m_reply != qobject_cast<QNetworkReply*>(sender())) {
        return;
    }

    bool error = false;
    const QByteArray response = QString::fromUtf8(m_reply->readAll()).toUtf8();

    if (m_reply->error() != QNetworkReply::NoError ||
        !response.startsWith(QByteArray("[Adblock")) ||
        !saveDownloadedData(response)
       ) {
        error = true;
    }

    m_reply->deleteLater();
    m_reply = 0;

    if (error) {
        emit subscriptionError(tr("Cannot load subscription!"));
        return;
    }

    loadSubscription(AdBlockManager::instance()->disabledRules());

    emit subscriptionUpdated();
    emit subscriptionChanged();
}

bool AdBlockSubscription::saveDownloadedData(const QByteArray &data)
{
    QSaveFile file(m_filePath);

    if (!file.open(QFile::WriteOnly)) {
        qWarning() << "AdBlockSubscription::" << __FUNCTION__ << "Unable to open adblock file for writing:" << m_filePath;
        return false;
    }

    // Write subscription header
    file.write(QString("Title: %1\nUrl: %2\n").arg(title(), url().toString()).toUtf8());
    file.write(data);
    file.commit();
    return true;
}

const AdBlockRule* AdBlockSubscription::rule(int offset) const
{
    if (!QzTools::containsIndex(m_rules, offset)) {
        return 0;
    }

    return m_rules[offset];
}

QVector<AdBlockRule*> AdBlockSubscription::allRules() const
{
    return m_rules;
}

const AdBlockRule* AdBlockSubscription::enableRule(int offset)
{
    if (!QzTools::containsIndex(m_rules, offset)) {
        return 0;
    }

    AdBlockRule* rule = m_rules[offset];
    rule->setEnabled(true);
    AdBlockManager::instance()->removeDisabledRule(rule->filter());

    emit subscriptionChanged();

    if (rule->isCssRule())
        mApp->reloadUserStyleSheet();

    return rule;
}

const AdBlockRule* AdBlockSubscription::disableRule(int offset)
{
    if (!QzTools::containsIndex(m_rules, offset)) {
        return 0;
    }

    AdBlockRule* rule = m_rules[offset];
    rule->setEnabled(false);
    AdBlockManager::instance()->addDisabledRule(rule->filter());

    emit subscriptionChanged();

    if (rule->isCssRule())
        mApp->reloadUserStyleSheet();

    return rule;
}

bool AdBlockSubscription::canEditRules() const
{
    return false;
}

bool AdBlockSubscription::canBeRemoved() const
{
    return true;
}

int AdBlockSubscription::addRule(AdBlockRule* rule)
{
    Q_UNUSED(rule)
    return -1;
}

bool AdBlockSubscription::removeRule(int offset)
{
    Q_UNUSED(offset)
    return false;
}

const AdBlockRule* AdBlockSubscription::replaceRule(AdBlockRule* rule, int offset)
{
    Q_UNUSED(rule)
    Q_UNUSED(offset)
    return 0;
}

AdBlockSubscription::~AdBlockSubscription()
{
    qDeleteAll(m_rules);
}

// AdBlockCustomList

AdBlockCustomList::AdBlockCustomList(QObject* parent)
    : AdBlockSubscription(tr("Custom Rules"), parent)
{
    setFilePath(DataPaths::currentProfilePath() + QLatin1String("/adblock/customlist.txt"));
}

void AdBlockCustomList::loadSubscription(const QStringList &disabledRules)
{
    // DuckDuckGo ad whitelist rules
    // They cannot be removed, but can be disabled.
    // Please consider not disabling them. Thanks!

    const QString ddg1 = QSL("@@||duckduckgo.com^$document");
    const QString ddg2 = QSL("duckduckgo.com#@#.has-ad");

    const QString rules = QzTools::readAllFileContents(filePath());

    QFile file(filePath());
    if (!file.exists()) {
        saveSubscription();
    }

    if (file.open(QFile::WriteOnly | QFile::Append)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        if (!rules.contains(ddg1 + QL1S("\n")))
            stream << ddg1 << endl;

        if (!rules.contains(QL1S("\n") + ddg2))
            stream << ddg2 << endl;
    }
    file.close();

    AdBlockSubscription::loadSubscription(disabledRules);
}

void AdBlockCustomList::saveSubscription()
{
    QFile file(filePath());

    if (!file.open(QFile::ReadWrite | QFile::Truncate)) {
        qWarning() << "AdBlockSubscription::" << __FUNCTION__ << "Unable to open adblock file for writing:" << filePath();
        return;
    }

    QTextStream textStream(&file);
    textStream.setCodec("UTF-8");
    textStream << "Title: " << title() << endl;
    textStream << "Url: " << url().toString() << endl;
    textStream << "[Adblock Plus 1.1.1]" << endl;

    foreach (const AdBlockRule* rule, m_rules) {
        textStream << rule->filter() << endl;
    }

    file.close();
}

bool AdBlockCustomList::canEditRules() const
{
    return true;
}

bool AdBlockCustomList::canBeRemoved() const
{
    return false;
}

bool AdBlockCustomList::containsFilter(const QString &filter) const
{
    foreach (const AdBlockRule* rule, m_rules) {
        if (rule->filter() == filter) {
            return true;
        }
    }

    return false;
}

bool AdBlockCustomList::removeFilter(const QString &filter)
{
    for (int i = 0; i < m_rules.count(); ++i) {
        const AdBlockRule* rule = m_rules.at(i);

        if (rule->filter() == filter) {
            return removeRule(i);
        }
    }

    return false;
}

int AdBlockCustomList::addRule(AdBlockRule* rule)
{
    m_rules.append(rule);

    emit subscriptionChanged();

    if (rule->isCssRule())
        mApp->reloadUserStyleSheet();

    return m_rules.count() - 1;
}

bool AdBlockCustomList::removeRule(int offset)
{
    if (!QzTools::containsIndex(m_rules, offset)) {
        return false;
    }

    AdBlockRule* rule = m_rules.at(offset);
    const QString filter = rule->filter();

    m_rules.remove(offset);

    emit subscriptionChanged();

    if (rule->isCssRule())
        mApp->reloadUserStyleSheet();

    AdBlockManager::instance()->removeDisabledRule(filter);

    delete rule;
    return true;
}

const AdBlockRule* AdBlockCustomList::replaceRule(AdBlockRule* rule, int offset)
{
    if (!QzTools::containsIndex(m_rules, offset)) {
        return 0;
    }

    AdBlockRule* oldRule = m_rules.at(offset);
    m_rules[offset] = rule;

    emit subscriptionChanged();

    if (rule->isCssRule() || oldRule->isCssRule())
        mApp->reloadUserStyleSheet();

    delete oldRule;
    return m_rules[offset];
}
