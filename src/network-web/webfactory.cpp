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

#include "network-web/webfactory.h"

#include "miscellaneous/application.h"

#include <QRegExp>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>

Q_GLOBAL_STATIC(WebFactory, qz_webfactory)


WebFactory::WebFactory()
	: m_escapes(QMap<QString, QString>()), m_deEscapes(QMap<QString, QString>()) {
}

WebFactory::~WebFactory() {
}

bool WebFactory::sendMessageViaEmail(const Message& message) {
	if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailEnabled)).toBool()) {
		const QString browser = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailExecutable)).toString();
		const QString arguments = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailArguments)).toString();
		return QProcess::startDetached(QString("\"") + browser + QSL("\" ") + arguments.arg(message.m_title,
		                               stripTags(message.m_contents)));
	}

	else {
		// Send it via mailto protocol.
		// NOTE: http://en.wikipedia.org/wiki/Mailto
		return QDesktopServices::openUrl(QString("mailto:?subject=%1&body=%2").arg(QString(QUrl::toPercentEncoding(message.m_title)),
		                                 QString(QUrl::toPercentEncoding(stripTags(message.m_contents)))));
	}
}

bool WebFactory::openUrlInExternalBrowser(const QString& url) {
	if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool()) {
		const QString browser = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString();
		const QString arguments = qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString();
		const QString call_line = "\"" + browser + "\" \"" + arguments.arg(url) + "\"";
		qDebug("Running command '%s'.", qPrintable(call_line));
		const bool result = QProcess::startDetached(call_line);

		if (!result) {
			qDebug("External web browser call failed.");
		}

		return result;
	}

	else {
		return QDesktopServices::openUrl(url);
	}
}

WebFactory* WebFactory::instance() {
	return qz_webfactory();
}

QString WebFactory::stripTags(QString text) {
	return text.remove(QRegExp(QSL("<[^>]*>")));
}

QString WebFactory::escapeHtml(const QString& html) {
	if (m_escapes.isEmpty()) {
		genereteEscapes();
	}

	QString output = html;
	QMapIterator<QString, QString> i(m_escapes);

	while (i.hasNext()) {
		i.next();
		output = output.replace(i.key(), i.value());
	}

	return output;
}

QString WebFactory::deEscapeHtml(const QString& text) {
	if (m_deEscapes.isEmpty()) {
		generateDeescapes();
	}

	QString output = text;
	QMapIterator<QString, QString> i(m_deEscapes);

	while (i.hasNext()) {
		i.next();
		output = output.replace(i.key(), i.value());
	}

	return output;
}

QString WebFactory::toSecondLevelDomain(const QUrl& url) {
	const QString top_level_domain = url.topLevelDomain();
	const QString url_host = url.host();

	if (top_level_domain.isEmpty() || url_host.isEmpty()) {
		return QString();
	}

	QString domain = url_host.left(url_host.size() - top_level_domain.size());

	if (domain.count(QL1C('.')) == 0) {
		return url_host;
	}

	while (domain.count(QL1C('.')) != 0) {
		domain = domain.mid(domain.indexOf(QL1C('.')) + 1);
	}

	return domain + top_level_domain;
}

void WebFactory::genereteEscapes() {
	m_escapes[QSL("&lt;")]     = QL1C('<');
	m_escapes[QSL("&gt;")]     = QL1C('>');
	m_escapes[QSL("&amp;")]    = QL1C('&');
	m_escapes[QSL("&quot;")]   = QL1C('\"');
	m_escapes[QSL("&nbsp;")]   = QL1C(' ');
	m_escapes[QSL("&plusmn;")]    = QSL("±");
	m_escapes[QSL("&times;")]  = QSL("×");
	m_escapes[QSL("&#039;")]   = QL1C('\'');
}

void WebFactory::generateDeescapes() {
	m_deEscapes[QSL("<")]  = QSL("&lt;");
	m_deEscapes[QSL(">")]  = QSL("&gt;");
	m_deEscapes[QSL("&")]  = QSL("&amp;");
	m_deEscapes[QSL("\"")]    = QSL("&quot;");
	m_deEscapes[QSL("±")]  = QSL("&plusmn;");
	m_deEscapes[QSL("×")]  = QSL("&times;");
	m_deEscapes[QSL("\'")] = QSL("&#039;");
}
