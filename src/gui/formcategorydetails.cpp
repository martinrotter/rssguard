#include "gui/formcategorydetails.h"

#include "gui/iconthemefactory.h"
#include "gui/feedsview.h"


FormCategoryDetails::FormCategoryDetails(FeedsView *parent)
  : QDialog(parent) {
  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::getInstance()->fromTheme("document-new"));
}

FormCategoryDetails::FormCategoryDetails(FeedsModelCategory *category,
                                         FeedsView *parent)
  :QDialog(parent) {
  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::getInstance()->fromTheme("document-new"));

}

FormCategoryDetails::~FormCategoryDetails() {
  qDebug("Destroying FormCategoryDetails instance.");
}
