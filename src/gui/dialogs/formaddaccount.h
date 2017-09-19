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

#ifndef FORMADDACCOUNT_H
#define FORMADDACCOUNT_H

#include <QDialog>

#include "ui_formaddaccount.h"

class ServiceEntryPoint;
class FeedsModel;

class FormAddAccount : public QDialog {
  Q_OBJECT

  public:
    explicit FormAddAccount(const QList<ServiceEntryPoint*>& entry_points, FeedsModel* model, QWidget* parent = 0);
    virtual ~FormAddAccount();

  private slots:
    void addSelectedAccount();
    void displayActiveEntryPointDetails();

  private:
    ServiceEntryPoint* selectedEntryPoint() const;

    void loadEntryPoints();

    QScopedPointer<Ui::FormAddAccount> m_ui;
    FeedsModel* m_model;

    QList<ServiceEntryPoint*> m_entryPoints;
};

#endif // FORMADDACCOUNT_H
