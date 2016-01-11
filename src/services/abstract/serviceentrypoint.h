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


class ServiceRoot;
class FeedsModel;

// TOP LEVEL class which provides basic information about the "service"
class ServiceEntryPoint {
  public:
    // Constructors.
    virtual ~ServiceEntryPoint();

    /////////////////////////////////////////
    // /* Members to override.
    /////////////////////////////////////////

    // Creates new service root item, which is ready to be added
    // into the model. This method can for example display
    // some kind of first-time configuration dialog inside itself
    // before returning the root item.
    // Returns NULL if initialization of new root cannot be done.
    virtual ServiceRoot *createNewRoot() = 0;

    // Performs initialization of all service accounts created using this entry
    // point from persistent DB.
    // Returns list of root nodes which will be afterwards added
    // to the global feed model.
    virtual QList<ServiceRoot*> initializeSubtree() const = 0;

    // Can this service account be added just once?
    // NOTE: This is true particularly for "standard" service
    // which operates with normal RSS/ATOM feeds.
    virtual bool isSingleInstanceService() const = 0;

    // Human readable service name, for example "TT-RSS".
    virtual QString name() const = 0;

    // Some arbitrary string.
    // NOTE: Keep in sync with ServiceRoot::code().
    virtual QString code() const = 0;

    // Human readable service description, for example "Services which offers TT-RSS integration.".
    virtual QString description() const = 0;

    // Version of the service, using of semantic versioning is recommended.
    virtual QString version() const = 0;

    // Author of the service.
    virtual QString author() const = 0;

    // Icon of the service.
    virtual QIcon icon() const = 0;

    /////////////////////////////////////////
    // Members to override. */
    /////////////////////////////////////////
};

#endif // SERVICE_H
