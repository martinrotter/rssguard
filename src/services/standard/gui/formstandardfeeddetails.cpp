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

#include "services/standard/gui/formstandardfeeddetails.h"

#include "services/standard/standardfeed.h"
#include "services/abstract/serviceroot.h"
#include "miscellaneous/application.h"


FormStandardFeedDetails::FormStandardFeedDetails(ServiceRoot* service_root, QWidget* parent)
	: FormFeedDetails(service_root, parent) {
}

void FormStandardFeedDetails::apply() {
	RootItem* parent = static_cast<RootItem*>(m_ui->m_cmbParentCategory->itemData(m_ui->m_cmbParentCategory->currentIndex()).value<void*>());
	StandardFeed::Type type = static_cast<StandardFeed::Type>(m_ui->m_cmbType->itemData(m_ui->m_cmbType->currentIndex()).value<int>());
	StandardFeed* new_feed = new StandardFeed();
	// Setup data for new_feed.
	new_feed->setTitle(m_ui->m_txtTitle->lineEdit()->text());
	new_feed->setCreationDate(QDateTime::currentDateTime());
	new_feed->setDescription(m_ui->m_txtDescription->lineEdit()->text());
	new_feed->setIcon(m_ui->m_btnIcon->icon());
	new_feed->setEncoding(m_ui->m_cmbEncoding->currentText());
	new_feed->setType(type);
	new_feed->setUrl(m_ui->m_txtUrl->lineEdit()->text());
	new_feed->setPasswordProtected(m_ui->m_gbAuthentication->isChecked());
	new_feed->setUsername(m_ui->m_txtUsername->lineEdit()->text());
	new_feed->setPassword(m_ui->m_txtPassword->lineEdit()->text());
	new_feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(
	                                m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
	new_feed->setAutoUpdateInitialInterval(m_ui->m_spinAutoUpdateInterval->value());

	if (m_editableFeed == nullptr) {
		// Add the feed.
		if (new_feed->addItself(parent)) {
			m_serviceRoot->requestItemReassignment(new_feed, parent);
			accept();
		}
		else {
			delete new_feed;
			qApp->showGuiMessage(tr("Cannot add feed"),
			                     tr("Feed was not added due to error."),
			                     QSystemTrayIcon::Critical, this, true);
		}
	}
	else {
		new_feed->setParent(parent);
		// Edit the feed.
		bool edited = qobject_cast<StandardFeed*>(m_editableFeed)->editItself(new_feed);

		if (edited) {
			m_serviceRoot->requestItemReassignment(m_editableFeed, new_feed->parent());
			accept();
		}
		else {
			qApp->showGuiMessage(tr("Cannot edit feed"),
			                     tr("Feed was not edited due to error."),
			                     QSystemTrayIcon::Critical, this, true);
		}

		delete new_feed;
	}
}

void FormStandardFeedDetails::setEditableFeed(Feed* editable_feed) {
	FormFeedDetails::setEditableFeed(editable_feed);
	StandardFeed* feed = qobject_cast<StandardFeed*>(editable_feed);
	m_ui->m_cmbType->setCurrentIndex(m_ui->m_cmbType->findData(QVariant::fromValue((int) feed->type())));
	m_ui->m_cmbEncoding->setCurrentIndex(m_ui->m_cmbEncoding->findData(feed->encoding(), Qt::DisplayRole, Qt::MatchFixedString));
	m_ui->m_gbAuthentication->setChecked(feed->passwordProtected());
	m_ui->m_txtUsername->lineEdit()->setText(feed->username());
	m_ui->m_txtPassword->lineEdit()->setText(feed->password());
}
