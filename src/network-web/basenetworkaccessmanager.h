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

#ifndef BASENETWORKACCESSMANAGER_H
#define BASENETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>


// This is base class for all network access managers.
class BaseNetworkAccessManager : public QNetworkAccessManager {
		Q_OBJECT

	public:
		// Constructors and desctructors.
		explicit BaseNetworkAccessManager(QObject* parent = 0);
		virtual ~BaseNetworkAccessManager();

	public slots:
		// Loads network settings for this instance.
		// NOTE: This sets up proxy settings.
		virtual void loadSettings();

	protected slots:
		// Called when some SSL-related errors are detected.
		void onSslErrors(QNetworkReply* reply, const QList<QSslError>& error);

	protected:
		// Creates custom request.
		QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* outgoingData);
};

#endif // BASENETWORKACCESSMANAGER_H
