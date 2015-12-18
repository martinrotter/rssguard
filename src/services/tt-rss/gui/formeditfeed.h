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

#ifndef FORMEDITFEED_H
#define FORMEDITFEED_H

#include <QDialog>

#include "ui_formeditfeed.h"


namespace Ui {
  class FormEditFeed;
}

class TtRssServiceRoot;
class TtRssFeed;
class Category;
class RootItem;

class FormEditFeed : public QDialog {
    Q_OBJECT

  public:
    explicit FormEditFeed(TtRssServiceRoot *root, QWidget *parent = 0);
    virtual ~FormEditFeed();

    int execForEdit(TtRssFeed *input_feed);
    int execForAdd();

  private slots:
    void performAction();
    void onAuthenticationSwitched();
    void onAutoUpdateTypeChanged(int new_index);
    void onUrlChanged(const QString &new_url);
    void onUsernameChanged(const QString &new_username);
    void onPasswordChanged(const QString &new_password);

  private:
    void initialize();
    void loadFeed(TtRssFeed *input_feed);
    void saveFeed();
    void loadCategories(const QList<Category*> categories, RootItem *root_item);

    TtRssServiceRoot *m_root;
    TtRssFeed *m_loadedFeed;
    Ui::FormEditFeed *m_ui;
};

#endif // FORMEDITFEED_H
