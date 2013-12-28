#include "gui/formcategorydetails.h"

#include "core/defs.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodel.h"
#include "gui/iconthemefactory.h"
#include "gui/feedsview.h"

#include <QPushButton>


FormCategoryDetails::FormCategoryDetails(FeedsView *parent)
  : QDialog(parent) {
  initialize(parent);
  loadCategories(parent->sourceModel()->getAllCategories().values(),
                 parent->sourceModel()->rootItem());

  setWindowTitle(tr("Add new category"));
}

FormCategoryDetails::FormCategoryDetails(FeedsModelCategory *category,
                                         FeedsView *parent)
  : QDialog(parent) {
  initialize(parent);
  loadCategories(parent->sourceModel()->getAllCategories().values(),
                 parent->sourceModel()->rootItem());

  setWindowTitle(tr("Edit existing category"));
}

FormCategoryDetails::~FormCategoryDetails() {
  qDebug("Destroying FormCategoryDetails instance.");
}

void FormCategoryDetails::initialize(FeedsView *view) {
  m_ui = new Ui::FormCategoryDetails();
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::getInstance()->fromTheme("document-new"));

  // Add button for obtaining data about feed from internet.
  m_btnObtainDetails = m_ui->m_buttonBox->addButton(tr("Get details via internet"),
                                                    QDialogButtonBox::ActionRole);
  m_btnObtainDetails->setIcon(IconThemeFactory::getInstance()->fromTheme("document-save"));
}

void FormCategoryDetails::loadCategories(const QList<FeedsModelCategory *> categories,
                                         FeedsModelRootItem *root_item) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     root_item->id());

  foreach (FeedsModelCategory *category, categories) {
    m_ui->m_cmbParentCategory->addItem(category->data(FDS_MODEL_TITLE_INDEX,
                                                      Qt::DecorationRole).value<QIcon>(),
                                       category->title(),
                                       category->id());
  }

}
