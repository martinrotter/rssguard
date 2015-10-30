// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef SERVICE_H
#define SERVICE_H

#include <QDialog>
#include <QIcon>
#include <QString>


// TOP LEVEL class which provides basic information about the "service"
class ServiceEntryPoint {
  public:
    // Constructors.
    explicit ServiceEntryPoint();
    virtual ~ServiceEntryPoint();

    // Must this service account be activated by default?
    // NOTE: This is true particularly for "standard" service
    // which operates with normal RSS/ATOM feeds.
    virtual bool isDefaultService() = 0;

    // Can this service account be added just once?
    // NOTE: This is true particularly for "standard" service
    // which operates with normal RSS/ATOM feeds.
    virtual bool isSingleInstanceService() = 0;

    // Can this service account be added by user via GUI?
    // NOTE: This is true particularly for "standard" service
    // which operates with normal RSS/ATOM feeds.
    virtual bool canBeAdded() = 0;

    // Can this service account by deleted by user via GUI?
    // NOTE: This is false particularly for "standard" service
    // which operates with normal RSS/ATOM feeds.
    virtual bool canBeDeleted() = 0;

    // Can properties of this service account be edited by user via GUI?
    virtual bool canBeEdited() = 0;

    // Human readable service name, for example "TT-RSS".
    virtual QString name() = 0;

    // Human readable service description, for example "Services which offers TT-RSS integration.".
    virtual QString description() = 0;

    // Version of the service, using of semantic versioning is recommended.
    virtual QString version() = 0;

    // Author of the service.
    virtual QString author() = 0;

    // Icon of the service.
    virtual QIcon icon() = 0;
};

#endif // SERVICE_H
