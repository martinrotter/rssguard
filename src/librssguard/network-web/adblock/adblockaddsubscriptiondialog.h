// For license of this file, see <project-root-folder>/LICENSE.md.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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
#include <QVector>

#include "ui_adblockaddsubscriptiondialog.h"

namespace Ui {
  class AdBlockAddSubscriptionDialog;
}

class AdBlockAddSubscriptionDialog : public QDialog {
  Q_OBJECT

  public:
    explicit AdBlockAddSubscriptionDialog(QWidget* parent = 0);
    virtual ~AdBlockAddSubscriptionDialog();

    QString title() const;
    QString url() const;

  private slots:
    void indexChanged(int index);
    void presetsEnabledChanged(bool enabled);

  private:
    Ui::AdBlockAddSubscriptionDialog* m_ui;

    struct Subscription {
      QString m_title;
      QString m_url;

      Subscription() {}

      Subscription(const QString& t, const QString& u) {
        m_title = t;
        m_url = u;
      }

    };

    QVector<Subscription> m_knownSubscriptions;
};

#endif // ADBLOCKADDSUBSCRIPTIONDIALOG_H
