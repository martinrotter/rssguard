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

#ifndef FORMFEEDDETAILS_H
#define FORMFEEDDETAILS_H

#include <QDialog>

#include "ui_formfeeddetails.h"


namespace Ui {
  class FormFeedDetails;
}

class ServiceRoot;
class Feed;
class Category;
class RootItem;

class FormFeedDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormFeedDetails(ServiceRoot *service_root, QWidget *parent = 0);
    virtual ~FormFeedDetails();

  public slots:
    // Executes add/edit standard feed dialog.
    int exec(Feed *input_feed, RootItem *parent_to_select, const QString &url = QString());

  protected slots:
    // Applies changes.
    // NOTE: This must be reimplemented in subclasses. Also this
    // base implementation must be called first.
    virtual void apply() = 0;

    void guessFeed();
    void guessIconOnly();

    // Trigerred when title/description/url/username/password changes.
    void onTitleChanged(const QString &new_title);
    void onDescriptionChanged(const QString &new_description);
    void onUrlChanged(const QString &new_url);
    void onUsernameChanged(const QString &new_username);
    void onPasswordChanged(const QString &new_password);
    void onAuthenticationSwitched();
    void onAutoUpdateTypeChanged(int new_index);

    // Icon selectors.
    void onNoIconSelected();
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  protected:
    // Sets the feed which will be edited.
    // NOTE: This must be reimplemented in subclasses. Also this
    // base implementation must be called first.
    void virtual setEditableFeed(Feed *editable_feed);

    // Creates needed connections.
    void createConnections();

    // Initializes the dialog.
    void initialize();

    // Loads categories into the dialog from the model.
    void loadCategories(const QList<Category*> categories, RootItem *root_item);

  protected:
    QScopedPointer<Ui::FormFeedDetails> m_ui;
    Feed *m_editableFeed;
    ServiceRoot *m_serviceRoot;

    QMenu *m_iconMenu;
    QAction *m_actionLoadIconFromFile;
    QAction *m_actionUseDefaultIcon;
    QAction *m_actionFetchIcon;
    QAction *m_actionNoIcon;
};

#endif // FORMFEEDDETAILS_H
