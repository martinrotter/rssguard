#ifndef FORMCATEGORYDETAILS_H
#define FORMCATEGORYDETAILS_H

#include "ui_formstandardcategorydetails.h"

#include <QDialog>


namespace Ui {
  class FormStandardCategoryDetails;
}

class FeedsModelCategory;
class FeedsModelStandardCategory;
class FeedsModel;
class FeedsModelRootItem;
class QMenu;
class QAction;

class FormStandardCategoryDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormStandardCategoryDetails(FeedsModel *model, QWidget *parent = 0);
    virtual ~FormStandardCategoryDetails();

  public slots:
    // Executes add/edit standard category dialog.
    int exec(FeedsModelStandardCategory *input_category);

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
    void setEditableCategory(FeedsModelStandardCategory *editable_category);

    // Initializes the dialog.
    void initialize();

    // Loads categories into the dialog + give root "category"
    // and make sure that no childs of input category (including)
    // input category are loaded.
    void loadCategories(const QList<FeedsModelCategory*> categories,
                        FeedsModelRootItem *root_item,
                        FeedsModelCategory *input_category);

  private:
    Ui::FormStandardCategoryDetails *m_ui;
    FeedsModelStandardCategory *m_editableCategory;
    FeedsModel *m_feedsModel;

    QMenu *m_iconMenu;
    QAction *m_actionLoadIconFromFile;
    QAction *m_actionUseDefaultIcon;
    QAction *m_actionNoIcon;
};

#endif // FORMCATEGORYDETAILS_H
