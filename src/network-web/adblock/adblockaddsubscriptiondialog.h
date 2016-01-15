// This file is part of RSS Guard.
//
// Copyright (C) 2014-2015 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#ifndef ADBLOCKADDSUBSCRIPTIONDIALOG_H
#define ADBLOCKADDSUBSCRIPTIONDIALOG_H

#include <QDialog>

#include "ui_adblockaddsubscriptiondialog.h"

#include <QVector>


namespace Ui {
  class AdBlockAddSubscriptionDialog;
}

class AdBlockAddSubscriptionDialog : public QDialog {
    Q_OBJECT

  public:
    // Constructors.
    explicit AdBlockAddSubscriptionDialog(QWidget *parent = 0);
    virtual ~AdBlockAddSubscriptionDialog();

    QString title() const;
    QString url() const;

  private slots:
    // Index of selected list changed.
    void onSubscriptionPresetChanged(int index);
    void checkInputs();

  private:
    void loadPresets();

    struct Subscription {
      public:
        QString m_title;
        QString m_url;

        explicit Subscription() {
        }

        explicit Subscription(const QString &title, const QString &url) {
          m_title = title;
          m_url = url;
        }
    };

    QScopedPointer<Ui::AdBlockAddSubscriptionDialog> m_ui;
    QVector<Subscription> m_knownSubscriptions;
};

#endif // ADBLOCKADDSUBSCRIPTIONDIALOG_H
