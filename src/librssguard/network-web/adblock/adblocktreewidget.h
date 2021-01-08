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

#ifndef ADBLOCKTREEWIDGET_H
#define ADBLOCKTREEWIDGET_H

#include "gui/treewidget.h"

class AdBlockSubscription;
class AdBlockRule;

class AdBlockTreeWidget : public TreeWidget {
  Q_OBJECT

  public:
    explicit AdBlockTreeWidget(AdBlockSubscription* subscription, QWidget* parent = 0);

    AdBlockSubscription* subscription() const;

    void showRule(const AdBlockRule* rule);
    void refresh();

  public slots:
    void addRule();
    void removeRule();

  private slots:
    void contextMenuRequested(const QPoint& pos);
    void itemChanged(QTreeWidgetItem* item);
    void copyFilter();

    void subscriptionUpdated();
    void subscriptionError(const QString& message);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);

  private:
    void adjustItemFeatures(QTreeWidgetItem* item, const AdBlockRule* rule);

    AdBlockSubscription* m_subscription;
    QTreeWidgetItem* m_topItem;
    QString m_ruleToBeSelected;
    bool m_itemChangingBlock;
};

#endif // ADBLOCKTREEWIDGET_H
