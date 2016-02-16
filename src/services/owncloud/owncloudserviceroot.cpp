// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/owncloud/owncloudserviceroot.h"

#include "definitions/definitions.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"


OwnCloudServiceRoot::OwnCloudServiceRoot(RootItem *parent) : ServiceRoot(parent), m_network(new OwnCloudNetworkFactory()) {
}

OwnCloudServiceRoot::~OwnCloudServiceRoot() {
  delete m_network;
}

bool OwnCloudServiceRoot::canBeEdited() const {
  return true;
}

bool OwnCloudServiceRoot::canBeDeleted() const {
  return true;
}

bool OwnCloudServiceRoot::editViaGui() {
  return false;
}

bool OwnCloudServiceRoot::deleteViaGui() {
  return false;
}

bool OwnCloudServiceRoot::supportsFeedAdding() const {
  // TODO: TODO
  return false;
}

bool OwnCloudServiceRoot::supportsCategoryAdding() const {
  // TODO: TODO
  return false;
}

QList<QAction*> OwnCloudServiceRoot::addItemMenu() {
  // TODO: TODO
  return QList<QAction*>();
}

QList<QAction*> OwnCloudServiceRoot::serviceMenu() {
  // TODO: TODO
  return QList<QAction*>();
}

RecycleBin *OwnCloudServiceRoot::recycleBin() const {
  // TODO: TODO
  return NULL;
}

void OwnCloudServiceRoot::start(bool freshly_activated) {
  // TODO: TODO
}

void OwnCloudServiceRoot::stop() {
  // TODO: TODO
}

QString OwnCloudServiceRoot::code() const {
  return SERVICE_CODE_OWNCLOUD;
}

bool OwnCloudServiceRoot::loadMessagesForItem(RootItem *item, QSqlTableModel *model) {
  // TODO: TODO
  return false;
}

OwnCloudNetworkFactory *OwnCloudServiceRoot::network() const {
  return m_network;
}

void OwnCloudServiceRoot::addNewFeed(const QString &url) {
  // TODO: TODO
}

void OwnCloudServiceRoot::addNewCategory() {
  // TODO: TODO
}
