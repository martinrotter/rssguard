#ifndef FORMCATEGORYDETAILS_H
#define FORMCATEGORYDETAILS_H

#include <QDialog>


class FeedsModelCategory;
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

  signals:

  public slots:

};

#endif // FORMCATEGORYDETAILS_H
