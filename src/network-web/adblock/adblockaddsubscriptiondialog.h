/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef ADBLOCKADDSUBSCRIPTIONDIALOG_H
#define ADBLOCKADDSUBSCRIPTIONDIALOG_H

#include <QDialog>
#include <QVector>

#include "qzcommon.h"

namespace Ui
{
class AdBlockAddSubscriptionDialog;
}

class QUPZILLA_EXPORT AdBlockAddSubscriptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdBlockAddSubscriptionDialog(QWidget* parent = 0);
    ~AdBlockAddSubscriptionDialog();

    QString title() const;
    QString url() const;

private slots:
    void indexChanged(int index);

private:
    Ui::AdBlockAddSubscriptionDialog* ui;

    struct Subscription {
        QString title;
        QString url;

        Subscription() {}

        Subscription(const QString &t, const QString &u) {
            title = t;
            url = u;
        }
    };

    QVector<Subscription> m_knownSubscriptions;
};

#endif // ADBLOCKADDSUBSCRIPTIONDIALOG_H
