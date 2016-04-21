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

#ifndef FORMEDITOWNCLOUDFEED_H
#define FORMEDITOWNCLOUDFEED_H

#include <QDialog>

#include "ui_formeditowncloudfeed.h"


namespace Ui {
  class FormEditOwnCloudFeed;
}

class OwnCloudServiceRoot;
class OwnCloudFeed;
class Category;
class RootItem;

class FormEditOwnCloudFeed : public QDialog {
    Q_OBJECT

  public:
    explicit FormEditOwnCloudFeed(OwnCloudServiceRoot *root, QWidget *parent = 0);
    virtual ~FormEditOwnCloudFeed();

    int execForEdit(OwnCloudFeed *input_feed);
    int execForAdd(const QString &url);

  private slots:
    void performAction();
    void onAuthenticationSwitched();
    void onAutoUpdateTypeChanged(int new_index);
    void onUrlChanged(const QString &new_url);
    void onUsernameChanged(const QString &new_username);
    void onPasswordChanged(const QString &new_password);
    void onTitleChanged(const QString &title);

  private:
    void initialize();
    void loadFeed(OwnCloudFeed *input_feed);
    void saveFeed();
    void addNewFeed();
    void loadCategories(const QList<Category*> categories, RootItem *root_item);

    QScopedPointer<Ui::FormEditOwnCloudFeed> m_ui;
    OwnCloudServiceRoot *m_root;
    OwnCloudFeed *m_loadedFeed;
};

#endif // FORMEDITOWNCLOUDFEED_H
