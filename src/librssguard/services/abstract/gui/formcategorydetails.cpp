// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formcategorydetails.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "gui/reusable/baselineedit.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/gui/multifeededitcheckbox.h"
#include "services/abstract/rootitem.h"

#include "ui_formcategorydetails.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QImageReader>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>

FormCategoryDetails::FormCategoryDetails(ServiceRoot* service_root, RootItem* parent_to_select, QWidget* parent)
  : QDialog(parent), m_serviceRoot(service_root), m_parentToSelect(parent_to_select) {
  initialize();
  createConnections();

  // Initialize text boxes.
  onTitleChanged(QString());
  onDescriptionChanged(QString());
}

FormCategoryDetails::~FormCategoryDetails() {
  qDebugNN << LOGSEC_GUI << "Destroying FormCategoryDetails instance.";
}

void FormCategoryDetails::createConnections() {
  // General connections.
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormCategoryDetails::apply);
  connect(m_ui->m_txtTitle->lineEdit(), &BaseLineEdit::textChanged, this, &FormCategoryDetails::onTitleChanged);
  connect(m_ui->m_txtDescription->lineEdit(),
          &BaseLineEdit::textChanged,
          this,
          &FormCategoryDetails::onDescriptionChanged);

  // Icon connections.
  connect(m_actionLoadIconFromFile, &QAction::triggered, this, &FormCategoryDetails::onLoadIconFromFile);
  connect(m_actionUseDefaultIcon, &QAction::triggered, this, &FormCategoryDetails::onUseDefaultIcon);
}

bool FormCategoryDetails::isChangeAllowed(MultiFeedEditCheckBox* mcb) const {
  return !m_isBatchEdit || mcb->isChecked();
}

void FormCategoryDetails::loadCategoryData() {
  Category* cat = category<Category>();

  if (m_isBatchEdit) {
    // We hook batch selectors.
    m_ui->m_mcbParent->addActionWidget(m_ui->m_cmbParentCategory);
    m_ui->m_mcbTitle->addActionWidget(m_ui->m_txtTitle);
    m_ui->m_mcbDescription->addActionWidget(m_ui->m_txtDescription);
    m_ui->m_mcbIcon->addActionWidget(m_ui->m_btnIcon);
  }
  else {
    // We hide batch selectors.
    for (auto* cb : findChildren<MultiFeedEditCheckBox*>()) {
      cb->hide();
    }
  }

  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot, cat);

  if (m_creatingNew) {
    GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("folder")), tr("Add new category"));

    // Make sure that "default" icon is used as the default option for new
    // categories.
    m_actionUseDefaultIcon->trigger();

    // Load parent from suggested item.
    if (m_parentToSelect != nullptr) {
      if (m_parentToSelect->kind() == RootItem::Kind::Category) {
        m_ui->m_cmbParentCategory
          ->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue(m_parentToSelect)));
      }
      else if (m_parentToSelect->kind() == RootItem::Kind::Feed) {
        int target_item = m_ui->m_cmbParentCategory->findData(QVariant::fromValue(m_parentToSelect->parent()));

        if (target_item >= 0) {
          m_ui->m_cmbParentCategory->setCurrentIndex(target_item);
        }
      }
    }
  }
  else {
    if (!m_isBatchEdit) {
      GuiUtilities::applyDialogProperties(*this, cat->fullIcon(), tr("Edit \"%1\"").arg(cat->title()));
    }
    else {
      GuiUtilities::applyDialogProperties(*this,
                                          qApp->icons()->fromTheme(QSL("folder")),
                                          tr("Edit %n categories", nullptr, m_categories.size()));
    }

    m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue(cat->parent())));
  }

  m_ui->m_txtTitle->lineEdit()->setText(cat->title());
  m_ui->m_txtDescription->lineEdit()->setText(cat->description());
  m_ui->m_btnIcon->setIcon(cat->icon());

  m_ui->m_txtTitle->lineEdit()->setFocus();
}

void FormCategoryDetails::apply() {
  QList<Category*> cats = categories<Category>();
  RootItem* parent = m_ui->m_cmbParentCategory->currentData().value<RootItem*>();
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  for (Category* cat : cats) {
    if (isChangeAllowed(m_ui->m_mcbTitle)) {
      cat->setTitle(m_ui->m_txtTitle->lineEdit()->text());
    }

    if (isChangeAllowed(m_ui->m_mcbDescription)) {
      cat->setDescription(m_ui->m_txtDescription->lineEdit()->text());
    }

    if (isChangeAllowed(m_ui->m_mcbIcon)) {
      cat->setIcon(m_ui->m_btnIcon->icon());
    }

    int new_parent_id;

    if (isChangeAllowed(m_ui->m_mcbParent)) {
      new_parent_id = parent->id();
    }
    else {
      new_parent_id = cat->parent()->id();
    }

    try {
      DatabaseQueries::createOverwriteCategory(database, cat, m_serviceRoot->accountId(), new_parent_id);
    }
    catch (const ApplicationException& ex) {
      qFatal("Cannot save category: '%s'.", qPrintable(ex.message()));
    }

    if (isChangeAllowed(m_ui->m_mcbParent)) {
      m_serviceRoot->requestItemReassignment(cat, parent);
    }

    if (m_creatingNew) {
      m_serviceRoot->requestItemExpand({parent}, true);
    }
  }

  m_serviceRoot->itemChanged(categories<RootItem>());
  accept();
}

void FormCategoryDetails::onTitleChanged(const QString& new_title) {
  if (!new_title.simplified().isEmpty()) {
    m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(true);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::StatusType::Ok, tr("Category name is ok."));
  }
  else {
    m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::StatusType::Error, tr("Category name is too short."));
  }
}

void FormCategoryDetails::onDescriptionChanged(const QString& new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Warning, tr("Description is empty."));
  }
  else {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Ok, tr("The description is ok."));
  }
}

void FormCategoryDetails::onLoadIconFromFile() {
  auto supported_formats = QImageReader::supportedImageFormats();
  auto prefixed_formats = boolinq::from(supported_formats)
                            .select([](const QByteArray& frmt) {
                              return QSL("*.%1").arg(QString::fromLocal8Bit(frmt));
                            })
                            .toStdList();

  QStringList list_formats = FROM_STD_LIST(QStringList, prefixed_formats);

  QFileDialog dialog(this,
                     tr("Select icon file for the category"),
                     qApp->homeFolder(),
                     tr("Images (%1)").arg(list_formats.join(QL1C(' '))));

  dialog.setFileMode(QFileDialog::FileMode::ExistingFile);
  dialog.setWindowIcon(qApp->icons()->fromTheme(QSL("image-x-generic")));
  dialog.setOptions(QFileDialog::Option::DontUseNativeDialog | QFileDialog::Option::ReadOnly);
  dialog.setViewMode(QFileDialog::ViewMode::Detail);
  dialog.setLabelText(QFileDialog::DialogLabel::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::DialogLabel::Reject, tr("Cancel"));

  //: Label to describe the folder for icon file selection dialog.
  dialog.setLabelText(QFileDialog::DialogLabel::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::DialogLabel::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::DialogLabel::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::DialogCode::Accepted) {
    m_ui->m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void FormCategoryDetails::onUseDefaultIcon() {
  m_ui->m_btnIcon->setIcon(QIcon());
}

void FormCategoryDetails::initialize() {
  m_ui.reset(new Ui::FormCategoryDetails());
  m_ui->setupUi(this);

  // Set text boxes.
  m_ui->m_txtTitle->lineEdit()->setPlaceholderText(tr("Category title"));
  m_ui->m_txtTitle->lineEdit()->setToolTip(tr("Set title for your category."));
  m_ui->m_txtDescription->lineEdit()->setPlaceholderText(tr("Category description"));
  m_ui->m_txtDescription->lineEdit()->setToolTip(tr("Set description for your category."));

  // Setup button box.
  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile =
    new QAction(qApp->icons()->fromTheme(QSL("image-x-generic")), tr("Load icon from file..."), this);
  m_actionUseDefaultIcon =
    new QAction(qApp->icons()->fromTheme(QSL("folder")), tr("Use default icon from icon theme"), this);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  m_ui->m_btnIcon->setMenu(m_iconMenu);

  // Setup tab order.
  setTabOrder(m_ui->m_cmbParentCategory, m_ui->m_txtTitle->lineEdit());
  setTabOrder(m_ui->m_txtTitle->lineEdit(), m_ui->m_txtDescription->lineEdit());
  setTabOrder(m_ui->m_txtDescription->lineEdit(), m_ui->m_btnIcon);
  setTabOrder(m_ui->m_btnIcon, m_ui->m_buttonBox);
  m_ui->m_txtTitle->lineEdit()->setFocus(Qt::FocusReason::TabFocusReason);
}

void FormCategoryDetails::loadCategories(const QList<Category*>& categories,
                                         RootItem* root_item,
                                         Category* input_category) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(), root_item->title(), QVariant::fromValue(root_item));

  for (Category* category : categories) {
    if (input_category != nullptr && (category == input_category || category->isChildOf(input_category))) {
      // This category cannot be selected as the new
      // parent for currently edited category, so
      // don't add it.
      continue;
    }

    m_ui->m_cmbParentCategory->addItem(category->data(FDS_MODEL_TITLE_INDEX, Qt::DecorationRole).value<QIcon>(),
                                       category->title(),
                                       QVariant::fromValue(category));
  }
}
