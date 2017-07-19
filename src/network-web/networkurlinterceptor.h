/* ============================================================
* QupZilla - QtWebEngine based browser
* Copyright (C) 2015 David Rosca <nowrep@gmail.com>
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

#ifndef NETWORKURLINTERCEPTOR_H
#define NETWORKURLINTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>

#include "qzcommon.h"

class UrlInterceptor;

class QUPZILLA_EXPORT NetworkUrlInterceptor : public QWebEngineUrlRequestInterceptor
{
public:
    explicit NetworkUrlInterceptor(QObject* parent = Q_NULLPTR);

    void interceptRequest(QWebEngineUrlRequestInfo &info) Q_DECL_OVERRIDE;

    void installUrlInterceptor(UrlInterceptor *interceptor);
    void removeUrlInterceptor(UrlInterceptor *interceptor);

    void loadSettings();

private:
    QList<UrlInterceptor*> m_interceptors;
    bool m_sendDNT;
};

#endif // NETWORKURLINTERCEPTOR_H
