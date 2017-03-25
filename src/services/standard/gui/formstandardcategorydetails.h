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

#ifndef FORMCATEGORYDETAILS_H
#define FORMCATEGORYDETAILS_H

#include "ui_formstandardcategorydetails.h"

#include <QDialog>


namespace Ui {
  class FormStandardCategoryDetails;
}

class Category;
class StandardCategory;
class StandardServiceRoot;
class FeedsModel;
class RootItem;
class QMenu;
class QAction;

class FormStandardCategoryDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormStandardCategoryDetails(StandardServiceRoot *service_root, QWidget *parent = 0);
    virtual ~FormStandardCategoryDetails();

  public slots:
    // Executes add/edit standard category dialog.
    int exec(StandardCategory *input_category, RootItem *parent_to_select);

  protected slots:
    // Applies changes.
    void apply();

    // Trigerred when title/description changes.
    void onTitleChanged(const QString &new_title);
    void onDescriptionChanged(const QString &new_description);

    // Icon selectors.
    void onNoIconSelected();
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  protected:
    // Creates needed connections.
    void createConnections();

    // Sets the category which will be edited.
    void setEditableCategory(StandardCategory *editable_category);

    // Initializes the dialog.
    void initialize();

    // Loads categories into the dialog + give root "category"
    // and make sure that no childs of input category (including)
    // input category are loaded.
    void loadCategories(const QList<Category *> categories, RootItem *root_item, StandardCategory *input_category);

  private:
    QScopedPointer<Ui::FormStandardCategoryDetails> m_ui;
    StandardCategory *m_editableCategory;
    StandardServiceRoot *m_serviceRoot;

    QMenu *m_iconMenu;
    QAction *m_actionLoadIconFromFile;
    QAction *m_actionUseDefaultIcon;
    QAction *m_actionNoIcon;
};

#endif // FORMCATEGORYDETAILS_H
