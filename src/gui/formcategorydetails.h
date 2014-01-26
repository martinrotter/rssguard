#ifndef FORMCATEGORYDETAILS_H
#define FORMCATEGORYDETAILS_H

#include "ui_formcategorydetails.h"

#include <QDialog>


namespace Ui {
  class FormSettings;
}

class FeedsModelCategory;
class FeedsModelStandardCategory;
class FeedsModel;
class FeedsModelRootItem;


class FormCategoryDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    // This constructor is supposed to create new categories.
    explicit FormCategoryDetails(FeedsModel *model, QWidget *parent = 0);

    // Destructor.
    virtual ~FormCategoryDetails();

    // Creates needed connections.
    void createConnections();

  public slots:
    // Start dialog execution. If result is QDialog::Accepted,
    // then output_item contains added or edited category
    // and parent_item contains parent item of newly
    // created or edited category.
    // NOTE: Newly ADDED category is NOT added to the model NOR
    // to the database.
    // NOTE: Newly EDITED category IS COPY of its original.
    // SO NO ORIGINAL MODEL DATA ARE EDITED OR CHANGED.
    int exec(FeedsModelStandardCategory *input_category);

  protected slots:
    void apply();

    // Trigerred when title/description changes.
    void onTitleChanged(const QString &new_title);

  protected:
    // Sets the category which will be edited.
    // NOTE: This is used for editing categories.
    void setEditableCategory(FeedsModelStandardCategory *editable_category);

    // Initializes the dialog.
    void initialize();

    // Loads categories into the dialog.
    void loadCategories(const QList<FeedsModelCategory*> categories,
                        FeedsModelRootItem *root_item);

  private:
    Ui::FormCategoryDetails *m_ui;
    FeedsModelStandardCategory *m_editableCategory;
    FeedsModel *m_feedsModel;
};

#endif // FORMCATEGORYDETAILS_H
