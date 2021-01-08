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

#ifndef NETWORKURLINTERCEPTOR_H
#define NETWORKURLINTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>

class UrlInterceptor;

class NetworkUrlInterceptor : public QWebEngineUrlRequestInterceptor {
  Q_OBJECT

  public:
    explicit NetworkUrlInterceptor(QObject* parent = nullptr);

    void interceptRequest(QWebEngineUrlRequestInfo& info) Q_DECL_OVERRIDE;

    void installUrlInterceptor(UrlInterceptor* interceptor);
    void removeUrlInterceptor(UrlInterceptor* interceptor);

    void load();

  private:
    QList<UrlInterceptor*> m_interceptors;
    bool m_sendDnt;
};

#endif // NETWORKURLINTERCEPTOR_H
