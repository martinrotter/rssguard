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

#ifndef DISCOVERFEEDSBUTTON_H
#define DISCOVERFEEDSBUTTON_H

#include <QToolButton>


class DiscoverFeedsButton : public QToolButton {
    Q_OBJECT

  public:
    // Constructors.
    explicit DiscoverFeedsButton(QWidget *parent = 0);
    virtual ~DiscoverFeedsButton();

    // Feed addresses manipulators.
    void clearFeedAddresses();
    void setFeedAddresses(const QStringList &addresses);

  private slots:
    // User chose any of addresses.
    void linkTriggered(QAction *action);
    void fillMenu();

  private:
    QStringList m_addresses;
};

#endif // DISCOVERFEEDSBUTTON_H
