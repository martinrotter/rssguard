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

#ifndef FORMABOUT_H
#define FORMABOUT_H

#include "ui_formabout.h"

#include "definitions/definitions.h"

#include <QDialog>


namespace Ui {
  class FormAbout;
}

class FormAbout : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormAbout(QWidget *parent);
    virtual ~FormAbout();

  private:
    Ui::FormAbout *m_ui;
};


#endif // FORMABOUT_H
