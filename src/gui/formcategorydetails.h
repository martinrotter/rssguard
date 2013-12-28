#ifndef FORMCATEGORYDETAILS_H
#define FORMCATEGORYDETAILS_H

#include "ui_formcategorydetails.h"

#include <QDialog>


namespace Ui {
  class FormSettings;
}

class FeedsModelCategory;
class FeedsModelRootItem;
class FeedsView;

class FormCategoryDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    // This constructor is supposed to create new categories.
    explicit FormCategoryDetails(FeedsView *parent = 0);

    // This constructor is supposed to edit existing categories.
    explicit FormCategoryDetails(FeedsModelCategory *category,
                                 FeedsView *parent = 0);

    // Destructor.
    virtual ~FormCategoryDetails();

  protected:
    void initialize(FeedsView *view);

    // Loads categories into the dialog.
    void loadCategories(const QList<FeedsModelCategory*> categories,
                        FeedsModelRootItem *root_item);

  private:
    Ui::FormCategoryDetails *m_ui;
    QPushButton *m_btnObtainDetails;

};

#endif // FORMCATEGORYDETAILS_H
