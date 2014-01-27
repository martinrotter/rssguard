#include "gui/formstandardcategorydetails.h"

#include "core/defs.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodel.h"
#include "gui/iconthemefactory.h"
#include "gui/feedsview.h"
#include "gui/baselineedit.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QToolButton>
#include <QPushButton>


FormStandardCategoryDetails::FormStandardCategoryDetails(FeedsModel *model, QWidget *parent)
  : QDialog(parent),
    m_editableCategory(NULL),
    m_feedsModel(model)  {
  initialize();
  loadCategories(model->allCategories().values(),
                 model->rootItem());

  createConnections();

  // Initialize text boxes.
  onTitleChanged(QString());
}

FormStandardCategoryDetails::~FormStandardCategoryDetails() {
  qDebug("Destroying FormCategoryDetails instance.");
}

void FormStandardCategoryDetails::createConnections() {
  connect(m_ui->m_buttonBox, SIGNAL(accepted()),
          this, SLOT(apply()));
  connect(m_ui->m_txtTitle->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onTitleChanged(QString)));
}

void FormStandardCategoryDetails::setEditableCategory(FeedsModelStandardCategory *editable_category) {
  m_editableCategory = editable_category;

  m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) editable_category->parent())));
  m_ui->m_txtTitle->lineEdit()->setText(editable_category->title());
  m_ui->m_txtDescription->lineEdit()->setText(editable_category->description());
  m_ui->m_btnIcon->setIcon(editable_category->icon());
}

int FormStandardCategoryDetails::exec(FeedsModelStandardCategory *input_category) {
  if (input_category == NULL) {
    // User is adding new category.
    setWindowTitle(tr("Add new category"));
  }
  else {
    // This item cannot have set itself as the parent.
    m_ui->m_cmbParentCategory->removeItem(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) input_category)));

    // User is editing existing category.
    setWindowTitle(tr("Edit existing category"));
    setEditableCategory(input_category);
  }

  return QDialog::exec();
}

void FormStandardCategoryDetails::apply() {
  FeedsModelRootItem *parent = static_cast<FeedsModelRootItem*>(m_ui->m_cmbParentCategory->itemData(m_ui->m_cmbParentCategory->currentIndex()).value<void*>());
  FeedsModelStandardCategory *new_category = new FeedsModelStandardCategory();

  new_category->setTitle(m_ui->m_txtTitle->lineEdit()->text());
  new_category->setCreationDate(QDateTime::currentDateTime());
  new_category->setDescription(m_ui->m_txtDescription->lineEdit()->text());
  new_category->setIcon(m_ui->m_btnIcon->icon());
  new_category->setParent(parent);

  if (m_editableCategory == NULL) {
    // Add the category.
    if (m_feedsModel->addStandardCategory(new_category, parent)) {
      accept();
    }
    else {
      // TODO: hlasit chybu
    }
  }
  else {
    // TODO: edit category
    if (m_feedsModel->editStandardCategory(m_editableCategory, new_category)) {
      accept();
    }
    else {
      // TODO: hlasit chybu
    }
  }
}

void FormStandardCategoryDetails::onTitleChanged(const QString &new_title){
  if (new_title.size() >= MIN_CATEGORY_NAME_LENGTH) {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Ok, tr("This category name is ok."));
  }
  else {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Error, tr("This category name is too short."));
  }
}

void FormStandardCategoryDetails::initialize() {
  m_ui = new Ui::FormStandardCategoryDetails();
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::instance()->fromTheme("document-new"));

  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void FormStandardCategoryDetails::loadCategories(const QList<FeedsModelCategory *> categories,
                                         FeedsModelRootItem *root_item) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));
  // pro ziskani root_item static_cast<FeedsModelRootItem*>(itemData(i).value<void*>())
  // a stejně dole ve foreachi


  foreach (FeedsModelCategory *category, categories) {
    m_ui->m_cmbParentCategory->addItem(category->data(FDS_MODEL_TITLE_INDEX,
                                                      Qt::DecorationRole).value<QIcon>(),
                                       category->title(),
                                       QVariant::fromValue((void*) category));
  }
}
