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

#ifndef ADBLOCKSUBSCRIPTION_H
#define ADBLOCKSUBSCRIPTION_H

#include <QUrl>
#include <QVector>

#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblocksearchtree.h"

class QUrl;
class QNetworkReply;

class AdBlockSubscription : public QObject {
  Q_OBJECT

  public:
    explicit AdBlockSubscription(QString title, QObject* parent = nullptr);
    virtual ~AdBlockSubscription();

    QString title() const;

    QString filePath() const;
    void setFilePath(const QString& path);

    QUrl url() const;
    void setUrl(const QUrl& url);

    const AdBlockRule* rule(int offset) const;

    QVector<AdBlockRule*> allRules() const;

    const AdBlockRule* enableRule(int offset);
    const AdBlockRule* disableRule(int offset);
    virtual void loadSubscription(const QStringList& disabledRules);
    virtual void saveSubscription();
    virtual bool canEditRules() const;
    virtual bool canBeRemoved() const;
    virtual int addRule(AdBlockRule* rule);
    virtual bool removeRule(int offset);
    virtual const AdBlockRule* replaceRule(AdBlockRule* rule, int offset);

  public slots:
    void updateSubscription();

  signals:
    void subscriptionChanged();
    void subscriptionUpdated();
    void subscriptionError(const QString& message);

  protected slots:
    void subscriptionDownloaded();

  protected:
    virtual bool saveDownloadedData(const QByteArray& data);

  protected:
    QNetworkReply* m_reply;
    QVector<AdBlockRule*> m_rules;

  private:
    QString m_title;
    QString m_filePath;
    QUrl m_url;
    bool m_updated;
};

class AdBlockCustomList : public AdBlockSubscription {
  Q_OBJECT

  public:
    explicit AdBlockCustomList(QObject* parent = 0);

    void loadSubscription(const QStringList& disabledRules);
    void saveSubscription();

    bool canEditRules() const;
    bool canBeRemoved() const;

    bool containsFilter(const QString& filter) const;
    bool removeFilter(const QString& filter);

    int addRule(AdBlockRule* rule);
    bool removeRule(int offset);
    const AdBlockRule* replaceRule(AdBlockRule* rule, int offset);
};

#endif // ADBLOCKSUBSCRIPTION_H
