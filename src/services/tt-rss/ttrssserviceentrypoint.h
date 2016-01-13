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


#ifndef TTRSSSERVICEENTRYPOINT_H
#define TTRSSSERVICEENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"


class TtRssServiceEntryPoint : public ServiceEntryPoint {
  public:
    explicit TtRssServiceEntryPoint();
    virtual ~TtRssServiceEntryPoint();

    bool isSingleInstanceService() const;
    QString name() const;
    QString description() const;
    QString version() const;
    QString author() const;
    QIcon icon() const;
    QString code() const;

    ServiceRoot *createNewRoot();
    QList<ServiceRoot*> initializeSubtree() const;
};

#endif // TTRSSSERVICEENTRYPOINT_H
