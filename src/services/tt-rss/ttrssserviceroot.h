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

#ifndef TTRSSSERVICEROOT_H
#define TTRSSSERVICEROOT_H

#include "services/abstract/serviceroot.h"
#include "services/abstract/cacheforserviceroot.h"

#include <QCoreApplication>


class TtRssCategory;
class TtRssFeed;
class TtRssNetworkFactory;
class TtRssRecycleBin;

class TtRssServiceRoot : public ServiceRoot, public CacheForServiceRoot {
		Q_OBJECT

	public:
		explicit TtRssServiceRoot(RootItem* parent = nullptr);
		virtual ~TtRssServiceRoot();

		void start(bool freshly_activated);
		void stop();
		QString code() const;
		bool canBeEdited() const;
		bool canBeDeleted() const;
		bool editViaGui();
		bool deleteViaGui();
		bool markAsReadUnread(ReadStatus status);
		bool supportsFeedAdding() const;
		bool supportsCategoryAdding() const;
		QVariant data(int column, int role) const;
		QList<QAction*> serviceMenu();
		RecycleBin* recycleBin() const;
		void saveAllCachedData();

		bool onBeforeSetMessagesRead(RootItem* selected_item, const QList<Message>& messages, ReadStatus read);
		bool onBeforeSwitchMessageImportance(RootItem* selected_item, const QList<ImportanceChange>& changes);

		// Access to network.
		TtRssNetworkFactory* network() const;

		void saveAccountDataToDatabase();
		void updateTitle();

	public slots:
		void addNewFeed(const QString& url = QString());
		void addNewCategory();

	private:
		RootItem* obtainNewTreeForSyncIn() const;
		QMap<int, QVariant> storeCustomFeedsData();
		void restoreCustomFeedsData(const QMap<int, QVariant>& data, const QHash<int, Feed*>& feeds);

		void loadFromDatabase();

		TtRssRecycleBin* m_recycleBin;
		QAction* m_actionSyncIn;
		QList<QAction*> m_serviceMenu;
		TtRssNetworkFactory* m_network;
};

#endif // TTRSSSERVICEROOT_H
