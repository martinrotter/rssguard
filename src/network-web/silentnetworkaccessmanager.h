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

#ifndef SILENTNETWORKACCESSMANAGER_H
#define SILENTNETWORKACCESSMANAGER_H

#include "network-web/basenetworkaccessmanager.h"

#include <QPointer>


// Network manager used for more communication for feeds.
// This network manager does not provide any GUI interaction options.
class SilentNetworkAccessManager : public BaseNetworkAccessManager {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit SilentNetworkAccessManager(QObject *parent = 0);
    virtual ~SilentNetworkAccessManager();

    // Returns pointer to global silent network manager
    static SilentNetworkAccessManager *instance();

  public slots:
    // This cannot do any GUI stuff.
    void onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);

  private:
    static QPointer<SilentNetworkAccessManager> s_instance;
};

#endif // SILENTNETWORKACCESSMANAGER_H
