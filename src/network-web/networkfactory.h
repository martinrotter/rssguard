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

#ifndef NETWORKFACTORY_H
#define NETWORKFACTORY_H

#include <QNetworkReply>
#include <QCoreApplication>
#include <QPair>
#include <QVariant>


typedef QPair<QNetworkReply::NetworkError, QVariant> NetworkResult;
class Downloader;

class NetworkFactory {
		Q_DECLARE_TR_FUNCTIONS(NetworkFactory)

	private:
		// Constructor.
		explicit NetworkFactory();

	public:
		static QStringList extractFeedLinksFromHtmlPage(const QUrl& url, const QString& html);

		// Returns human readable text for given network error.
		static QString networkErrorText(QNetworkReply::NetworkError error_code);

		// Performs SYNCHRONOUS download if favicon for the site,
		// given URL belongs to.
		static QNetworkReply::NetworkError downloadIcon(const QList<QString>& urls, int timeout, QIcon& output);

    static Downloader* performAsyncNetworkOperation(const QString& url, int timeout, const QByteArray& input_data,
                                                    const QString& input_content_type,
                                                    QNetworkAccessManager::Operation operation,
                                                    bool protected_contents = false, const QString& username = QString(),
                                                    const QString& password = QString(), bool set_basic_header = false);

		static NetworkResult performNetworkOperation(const QString& url, int timeout, const QByteArray& input_data,
		                                             const QString& input_content_type, QByteArray& output,
		                                             QNetworkAccessManager::Operation operation,
		                                             bool protected_contents = false, const QString& username = QString(),
		                                             const QString& password = QString(), bool set_basic_header = false);

		static NetworkResult downloadFeedFile(const QString& url, int timeout, QByteArray& output,
		                                      bool protected_contents = false, const QString& username = QString(),
		                                      const QString& password = QString());
};

#endif // NETWORKFACTORY_H
