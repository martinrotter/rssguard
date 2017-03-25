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

#ifndef FORMTTRSSFEEDDETAILS_H
#define FORMTTRSSFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"


class FormTtRssFeedDetails : public FormFeedDetails {
    Q_OBJECT

  public:
    explicit FormTtRssFeedDetails(ServiceRoot *service_root, QWidget *parent = 0);

    // FormFeedDetails interface
  protected slots:
    void apply();

  protected:
    void setEditableFeed(Feed *editable_feed);
};

#endif // FORMTTRSSFEEDDETAILS_H
