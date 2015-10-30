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

#ifndef FORMSTANDARDFEEDDETAILS_H
#define FORMSTANDARDFEEDDETAILS_H

#include <QDialog>

#include "ui_formstandardfeeddetails.h"


namespace Ui {
  class FormStandardFeedDetails;
}

class FeedsModel;
class StandardFeed;
class StandardCategory;
class RootItem;

class FormStandardFeedDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormStandardFeedDetails(FeedsModel *model, QWidget *parent = 0);
    virtual ~FormStandardFeedDetails();

  public slots:
    // Executes add/edit standard feed dialog.
    int exec(StandardFeed *input_feed, RootItem *parent_to_select);

  protected slots:
    // Applies changes.
    void apply();
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

    // Check if "OK" button can be enabled or not.
    void checkOkButtonEnabled();

    // Icon selectors.
    void onNoIconSelected();
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  protected:
    // Creates needed connections.
    void createConnections();

    // Sets the feed which will be edited.
    void setEditableFeed(StandardFeed *editable_feed);

    // Initializes the dialog.
    void initialize();

    // Loads categories into the dialog from the model.
    void loadCategories(const QList<StandardCategory*> categories,
                        RootItem *root_item);

  private:
    Ui::FormStandardFeedDetails *m_ui;
    StandardFeed *m_editableFeed;
    FeedsModel *m_feedsModel;

    QMenu *m_iconMenu;
    QAction *m_actionLoadIconFromFile;
    QAction *m_actionUseDefaultIcon;
    QAction *m_actionFetchIcon;
    QAction *m_actionNoIcon;
};

#endif // FORMSTANDARDFEEDDETAILS_H
