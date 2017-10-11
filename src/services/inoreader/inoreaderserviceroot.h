// This file is part of RSS Guard.

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

#ifndef INOREADERSERVICEROOT_H
#define INOREADERSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class InoreaderNetworkFactory;

class InoreaderServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    explicit InoreaderServiceRoot(InoreaderNetworkFactory* network, RootItem* parent = nullptr);
    virtual ~InoreaderServiceRoot();

    void saveAccountDataToDatabase();

    void setNetwork(InoreaderNetworkFactory* network);
    InoreaderNetworkFactory* network() const;

    bool canBeEdited() const;
    bool editViaGui();
    bool canBeDeleted() const;
    bool deleteViaGui();
    bool supportsFeedAdding() const;
    bool supportsCategoryAdding() const;
    void start(bool freshly_activated);
    void stop();
    QString code() const;

    QString additionalTooltip() const;

    RootItem* obtainNewTreeForSyncIn() const;

    void saveAllCachedData(bool async = true);

  public slots:
    void addNewFeed(const QString& url);
    void addNewCategory();
    void updateTitle();

  private:
    void loadFromDatabase();
    QList<QAction*> serviceMenu();

  private:
    QList<QAction*> m_serviceMenu;
    InoreaderNetworkFactory* m_network;
};

inline void InoreaderServiceRoot::setNetwork(InoreaderNetworkFactory* network) {
  m_network = network;
}

inline InoreaderNetworkFactory* InoreaderServiceRoot::network() const {
  return m_network;
}

#endif // INOREADERSERVICEROOT_H
