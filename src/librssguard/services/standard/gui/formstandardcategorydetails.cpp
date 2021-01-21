// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/formstandardcategorydetails.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/rootitem.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardserviceroot.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>

FormStandardCategoryDetails::FormStandardCategoryDetails(StandardServiceRoot* service_root, QWidget* parent)
  : QDialog(parent), m_editableCategory(nullptr), m_serviceRoot(service_root) {
  initialize();
  createConnections();

  // Initialize text boxes.
  onTitleChanged(QString());
  onDescriptionChanged(QString());
}

FormStandardCategoryDetails::~FormStandardCategoryDetails() {
  qDebugNN << LOGSEC_GUI << "Destroying FormCategoryDetails instance.";
}

void FormStandardCategoryDetails::createConnections() {
  // General connections.
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormStandardCategoryDetails::apply);
  connect(m_ui->m_txtTitle->lineEdit(), &BaseLineEdit::textChanged,
          this, &FormStandardCategoryDetails::onTitleChanged);
  connect(m_ui->m_txtDescription->lineEdit(), &BaseLineEdit::textChanged,
          this, &FormStandardCategoryDetails::onDescriptionChanged);

  // Icon connections.
  connect(m_actionLoadIconFromFile, &QAction::triggered, this, &FormStandardCategoryDetails::onLoadIconFromFile);
  connect(m_actionUseDefaultIcon, &QAction::triggered, this, &FormStandardCategoryDetails::onUseDefaultIcon);
}

void FormStandardCategoryDetails::setEditableCategory(StandardCategory* editable_category) {
  m_editableCategory = editable_category;
  m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) editable_category->parent())));
  m_ui->m_txtTitle->lineEdit()->setText(editable_category->title());
  m_ui->m_txtDescription->lineEdit()->setText(editable_category->description());
  m_ui->m_btnIcon->setIcon(editable_category->icon());
}

int FormStandardCategoryDetails::addEditCategory(StandardCategory* input_category, RootItem* parent_to_select) {
  // Load categories.
  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot, input_category);

  if (input_category == nullptr) {
    // User is adding new category.
    setWindowTitle(tr("Add new category"));

    // Make sure that "default" icon is used as the default option for new
    // categories.
    m_actionUseDefaultIcon->trigger();

    // Load parent from suggested item.
    if (parent_to_select != nullptr) {
      if (parent_to_select->kind() == RootItem::Kind::Category) {
        m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) parent_to_select)));
      }
      else if (parent_to_select->kind() == RootItem::Kind::Feed) {
        int target_item = m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) parent_to_select->parent()));

        if (target_item >= 0) {
          m_ui->m_cmbParentCategory->setCurrentIndex(target_item);
        }
      }
    }
  }
  else {
    // User is editing existing category.
    setWindowTitle(tr("Edit existing category"));
    setEditableCategory(input_category);
  }

  // Run the dialog.
  return QDialog::exec();
}

void FormStandardCategoryDetails::apply() {
  RootItem* parent = static_cast<RootItem*>(m_ui->m_cmbParentCategory->itemData(m_ui->m_cmbParentCategory->currentIndex()).value<void*>());
  auto* new_category = new StandardCategory();

  new_category->setTitle(m_ui->m_txtTitle->lineEdit()->text());
  new_category->setCreationDate(QDateTime::currentDateTime());
  new_category->setDescription(m_ui->m_txtDescription->lineEdit()->text());
  new_category->setIcon(m_ui->m_btnIcon->icon());

  if (m_editableCategory == nullptr) {
    // Add the category.
    if (new_category->addItself(parent)) {
      m_serviceRoot->requestItemReassignment(new_category, parent);
      accept();
    }
    else {
      delete new_category;
      qApp->showGuiMessage(tr("Cannot add category"),
                           tr("Category was not added due to error."),
                           QSystemTrayIcon::MessageIcon::Critical,
                           qApp->mainFormWidget(), true);
    }
  }
  else {
    new_category->setParent(parent);
    bool edited = m_editableCategory->editItself(new_category);

    if (edited) {
      m_serviceRoot->requestItemReassignment(m_editableCategory, new_category->parent());
      accept();
    }
    else {
      qApp->showGuiMessage(tr("Cannot edit category"),
                           tr("Category was not edited due to error."),
                           QSystemTrayIcon::Critical, this, true);
    }

    delete new_category;
  }
}

void FormStandardCategoryDetails::onTitleChanged(const QString& new_title) {
  if (new_title.simplified().size() >= MIN_CATEGORY_NAME_LENGTH) {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::StatusType::Ok, tr("Category name is ok."));
  }
  else {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::StatusType::Error, tr("Category name is too short."));
  }
}

void FormStandardCategoryDetails::onDescriptionChanged(const QString& new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Warning, tr("Description is empty."));
  }
  else {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Ok, tr("The description is ok."));
  }
}

void FormStandardCategoryDetails::onLoadIconFromFile() {
  QFileDialog dialog(this, tr("Select icon file for the category"),
                     qApp->homeFolder(), tr("Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)"));

  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setWindowIcon(qApp->icons()->fromTheme(QSL("image-x-generic")));
  dialog.setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly);
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setLabelText(QFileDialog::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));

  //: Label to describe the folder for icon file selection dialog.
  dialog.setLabelText(QFileDialog::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::Accepted) {
    m_ui->m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void FormStandardCategoryDetails::onUseDefaultIcon() {
  m_ui->m_btnIcon->setIcon(QIcon());
}

void FormStandardCategoryDetails::initialize() {
  m_ui.reset(new Ui::FormStandardCategoryDetails());
  m_ui->setupUi(this);

  // Set text boxes.
  m_ui->m_txtTitle->lineEdit()->setPlaceholderText(tr("Category title"));
  m_ui->m_txtTitle->lineEdit()->setToolTip(tr("Set title for your category."));
  m_ui->m_txtDescription->lineEdit()->setPlaceholderText(tr("Category description"));
  m_ui->m_txtDescription->lineEdit()->setToolTip(tr("Set description for your category."));

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("folder")));

  // Setup button box.
  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile = new QAction(qApp->icons()->fromTheme(QSL("image-x-generic")),
                                         tr("Load icon from file..."),
                                         this);
  m_actionUseDefaultIcon = new QAction(qApp->icons()->fromTheme(QSL("folder")),
                                       tr("Use default icon from icon theme"),
                                       this);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  m_ui->m_btnIcon->setMenu(m_iconMenu);

  // Setup tab order.
  setTabOrder(m_ui->m_cmbParentCategory, m_ui->m_txtTitle->lineEdit());
  setTabOrder(m_ui->m_txtTitle->lineEdit(), m_ui->m_txtDescription->lineEdit());
  setTabOrder(m_ui->m_txtDescription->lineEdit(), m_ui->m_btnIcon);
  setTabOrder(m_ui->m_btnIcon, m_ui->m_buttonBox);
  m_ui->m_txtTitle->lineEdit()->setFocus(Qt::TabFocusReason);
}

void FormStandardCategoryDetails::loadCategories(const QList<Category*>& categories,
                                                 RootItem* root_item,
                                                 StandardCategory* input_category) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));

  for (Category* category : categories) {
    if (input_category != nullptr && (category == input_category || category->isChildOf(input_category))) {
      // This category cannot be selected as the new
      // parent for currently edited category, so
      // don't add it.
      continue;
    }

    m_ui->m_cmbParentCategory->addItem(category->data(FDS_MODEL_TITLE_INDEX, Qt::DecorationRole).value<QIcon>(),
                                       category->title(),
                                       QVariant::fromValue((void*) category));
  }
}
