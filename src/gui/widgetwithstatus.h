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

#ifndef WIDGETWITHSTATUS_H
#define WIDGETWITHSTATUS_H

#include <QWidget>
#include <QIcon>


class PlainToolButton;
class QHBoxLayout;

class WidgetWithStatus : public QWidget {
    Q_OBJECT

  public:
    enum StatusType {
      Information,
      Warning,
      Error,
      Ok,
      Progress
    };

    // Constructors and destructors.
    explicit WidgetWithStatus(QWidget *parent);
    virtual ~WidgetWithStatus();

    // Sets custom status for this control.
    void setStatus(StatusType status, const QString &tooltip_text);

    inline StatusType status() const {
      return m_status;
    }

  protected:
    StatusType m_status;
    QWidget *m_wdgInput;
    PlainToolButton *m_btnStatus;
    QHBoxLayout *m_layout;

    QIcon m_iconProgress;
    QIcon m_iconInformation;
    QIcon m_iconWarning;
    QIcon m_iconError;
    QIcon m_iconOk;
};


#endif // WIDGETWITHSTATUS_H
