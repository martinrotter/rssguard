// For license of this file, see <project-root-folder>/LICENSE.md.

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
    explicit FormStandardCategoryDetails(StandardServiceRoot* service_root, QWidget* parent = nullptr);
    virtual ~FormStandardCategoryDetails();

  public slots:

    // Executes add/edit standard category dialog.
    int addEditCategory(StandardCategory* input_category, RootItem* parent_to_select);

  protected slots:

    // Applies changes.
    void apply();

    // Trigerred when title/description changes.
    void onTitleChanged(const QString& new_title);
    void onDescriptionChanged(const QString& new_description);

    // Icon selectors.
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  protected:

    // Creates needed connections.
    void createConnections();

    // Sets the category which will be edited.
    void setEditableCategory(StandardCategory* editable_category);

    // Initializes the dialog.
    void initialize();

    // Loads categories into the dialog + give root "category"
    // and make sure that no childs of input category (including)
    // input category are loaded.
    void loadCategories(const QList<Category*>& categories, RootItem* root_item, StandardCategory* input_category);

  private:
    QScopedPointer<Ui::FormStandardCategoryDetails> m_ui;
    StandardCategory* m_editableCategory;
    StandardServiceRoot* m_serviceRoot;
    QMenu* m_iconMenu{};
    QAction* m_actionLoadIconFromFile{};
    QAction* m_actionUseDefaultIcon{};
};

#endif // FORMCATEGORYDETAILS_H
