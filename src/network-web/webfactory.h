// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include <QObject>

#include "core/messagesmodel.h"

#include <QPointer>
#include <QMap>


class QWebEngineSettings;

class WebFactory : public QObject {
    Q_OBJECT

  public:
    // Destructor.
    virtual ~WebFactory();

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    // HTML entity escaping.
    QString escapeHtml(const QString &html);
    QString deEscapeHtml(const QString &text);

    // BUG: Version for Qt < 4.8 has one issue, it will wrongly
    // count .co.uk (and others) as second-level domain
    QString toSecondLevelDomain(const QUrl &url);

    // Singleton getter.
    static WebFactory *instance();

  public slots:
    // Opens given string URL in external browser.
    bool openUrlInExternalBrowser(const QString &url);
    bool sendMessageViaEmail(const Message &message);

  private:
    // Constructor.
    explicit WebFactory(QObject *parent = 0);

    // Escape sequences generators.
    void generetaEscapes();
    void generateDeescapes();

    QMap<QString, QString> m_escapes;
    QMap<QString, QString> m_deEscapes;

    // Singleton.
    static QPointer<WebFactory> s_instance;
};

#endif // WEBFACTORY_H
