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

#include "services/standard/atomparser.h"

#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "miscellaneous/application.h"

#include "exceptions/applicationexception.h"


AtomParser::AtomParser(const QString& data) : FeedParser(data), m_atomNamespace(QSL("http://www.w3.org/2005/Atom")) {
}

AtomParser::~AtomParser() {
}

QString AtomParser::feedAuthor() const {
	QDomNodeList authors = m_xml.documentElement().elementsByTagNameNS(m_atomNamespace, QSL("author"));
	QStringList author_str;

	for (int i = 0; i < authors.size(); i++) {
		QDomNodeList names = authors.at(i).toElement().elementsByTagNameNS(m_atomNamespace, QSL("name"));

		if (!names.isEmpty()) {
			const QString name = names.at(0).toElement().text();

			if (!name.isEmpty() && !author_str.contains(name)) {
				author_str.append(name);
			}
		}
	}

	return author_str.join(", ");
}

Message AtomParser::extractMessage(const QDomElement& msg_element, QDateTime current_time) const {
	Message new_message;
	QString title = textsFromPath(msg_element, m_atomNamespace, QSL("title"), true).join(QSL(", "));
	QString summary = textsFromPath(msg_element, m_atomNamespace, QSL("content"), true).join(QSL(", "));

	if (summary.isEmpty()) {
		summary = textsFromPath(msg_element, m_atomNamespace, QSL("summary"), true).join(QSL(", "));
	}

	// Now we obtained maximum of information for title & description.
	if (title.isEmpty() && summary.isEmpty()) {
		// BOTH title and description are empty, skip this message.
		throw new ApplicationException(QSL("Not enough data for the message."));
	}

	// Title is not empty, description does not matter.
	new_message.m_title = qApp->web()->stripTags(title);
	new_message.m_contents = summary;
	new_message.m_author = qApp->web()->escapeHtml(messageAuthor(msg_element));
	QString updated = textsFromPath(msg_element, m_atomNamespace, QSL("updated"), true).join(QSL(", "));
	// Deal with creation date.
	new_message.m_created = TextFactory::parseDateTime(updated);
	new_message.m_createdFromFeed = !new_message.m_created.isNull();

	if (!new_message.m_createdFromFeed) {
		// Date was NOT obtained from the feed, set current date as creation date for the message.
		new_message.m_created = current_time;
	}

	// Deal with links
	QDomNodeList elem_links = msg_element.toElement().elementsByTagNameNS(m_atomNamespace, QSL("link"));
	QString last_link_alternate, last_link_other;

	for (int i = 0; i < elem_links.size(); i++) {
		QDomElement link = elem_links.at(i).toElement();
		QString attribute = link.attribute(QSL("rel"));

		if (attribute == QSL("enclosure")) {
			new_message.m_enclosures.append(Enclosure(link.attribute(QSL("href")), link.attribute(QSL("type"))));
			qDebug("Adding enclosure '%s' for the message.", qPrintable(new_message.m_enclosures.last().m_url));
		}
		else if (attribute.isEmpty() || attribute == QSL("alternate")) {
			last_link_alternate = link.attribute(QSL("href"));
		}
		else {
			last_link_other = link.attribute(QSL("href"));
		}
	}

	if (!last_link_alternate.isEmpty()) {
		new_message.m_url = last_link_alternate;
	}
	else if (!last_link_other.isEmpty()) {
		new_message.m_url = last_link_other;
	}
	else if (!new_message.m_enclosures.isEmpty()) {
		new_message.m_url = new_message.m_enclosures.first().m_url;
	}

	return new_message;
}

QString AtomParser::messageAuthor(const QDomElement& msg_element) const {
	QDomNodeList authors = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("author"));
	QStringList author_str;

	for (int i = 0; i < authors.size(); i++) {
		QDomNodeList names = authors.at(i).toElement().elementsByTagNameNS(m_atomNamespace, QSL("name"));

		if (!names.isEmpty()) {
			author_str.append(names.at(0).toElement().text());
		}
	}

	return author_str.join(", ");
}

QDomNodeList AtomParser::messageElements() {
	return m_xml.elementsByTagNameNS(m_atomNamespace, QSL("entry"));
}
