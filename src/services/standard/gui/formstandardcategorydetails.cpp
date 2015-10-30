// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "services/standard/gui/formstandardcategorydetails.h"

#include "definitions/definitions.h"
#include "core/rootitem.h"
#include "core/feedsmodel.h"
#include "services/standard/standardcategory.h"
#include "miscellaneous/iconfactory.h"
#include "gui/feedsview.h"
#include "gui/baselineedit.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"
#include "gui/dialogs/formmain.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QFileDialog>


FormStandardCategoryDetails::FormStandardCategoryDetails(FeedsModel *model, QWidget *parent) : QDialog(parent),
    m_editableCategory(NULL),
    m_feedsModel(model)  {
  initialize();
  createConnections();

  // Initialize text boxes.
  onTitleChanged(QString());
  onDescriptionChanged(QString());
}

FormStandardCategoryDetails::~FormStandardCategoryDetails() {
  qDebug("Destroying FormCategoryDetails instance.");
}

void FormStandardCategoryDetails::createConnections() {
  // General connections.
  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(apply()));
  connect(m_ui->m_txtTitle->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onTitleChanged(QString)));
  connect(m_ui->m_txtDescription->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onDescriptionChanged(QString)));

  // Icon connections.
  connect(m_actionLoadIconFromFile, SIGNAL(triggered()), this, SLOT(onLoadIconFromFile()));
  connect(m_actionNoIcon, SIGNAL(triggered()), this, SLOT(onNoIconSelected()));
  connect(m_actionUseDefaultIcon, SIGNAL(triggered()), this, SLOT(onUseDefaultIcon()));
}

void FormStandardCategoryDetails::setEditableCategory(StandardCategory *editable_category) {
  m_editableCategory = editable_category;

  m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) editable_category->parent())));
  m_ui->m_txtTitle->lineEdit()->setText(editable_category->title());
  m_ui->m_txtDescription->lineEdit()->setText(editable_category->description());
  m_ui->m_btnIcon->setIcon(editable_category->icon());
}

int FormStandardCategoryDetails::exec(StandardCategory *input_category, RootItem *parent_to_select) {
  // Load categories.
  loadCategories(m_feedsModel->allCategories().values(), m_feedsModel->rootItem(), input_category);

  if (input_category == NULL) {
    // User is adding new category.
    setWindowTitle(tr("Add new category"));

    // Make sure that "default" icon is used as the default option for new
    // categories.
    m_actionUseDefaultIcon->trigger();

    // Load parent from suggested item.
    if (parent_to_select != NULL) {
      if (parent_to_select->kind() == RootItem::Cattegory) {
        m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) parent_to_select)));
      }
      else if (parent_to_select->kind() == RootItem::Feeed) {
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
  RootItem *parent = static_cast<RootItem*>(m_ui->m_cmbParentCategory->itemData(m_ui->m_cmbParentCategory->currentIndex()).value<void*>());
  StandardCategory *new_category = new StandardCategory();

  new_category->setTitle(m_ui->m_txtTitle->lineEdit()->text());
  new_category->setCreationDate(QDateTime::currentDateTime());
  new_category->setDescription(m_ui->m_txtDescription->lineEdit()->text());
  new_category->setIcon(m_ui->m_btnIcon->icon());
  new_category->setParent(parent);

  if (m_editableCategory == NULL) {
    // Add the category.
    if (m_feedsModel->addCategory(new_category, parent)) {
      accept();
    }
    else {
      qApp->showGuiMessage(tr("Cannot add category"),
                           tr("Category was not added due to error."),
                           QSystemTrayIcon::Critical,
                           qApp->mainForm(), true);
    }
  }
  else {
    if (m_feedsModel->editCategory(m_editableCategory, new_category)) {
      accept();
    }
    else {
      qApp->showGuiMessage(tr("Cannot edit category"),
                           tr("Category was not edited due to error."),
                           QSystemTrayIcon::Critical,
                           qApp->mainForm(), true);
    }
  }
}

void FormStandardCategoryDetails::onTitleChanged(const QString &new_title){
  if (new_title.simplified().size() >= MIN_CATEGORY_NAME_LENGTH) {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::Ok, tr("Category name is ok."));
  }
  else {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_ui->m_txtTitle->setStatus(WidgetWithStatus::Error, tr("Category name is too short."));
  }
}

void FormStandardCategoryDetails::onDescriptionChanged(const QString &new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Warning, tr("Description is empty."));
  }
  else {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Ok, tr("The description is ok."));
  }
}

void FormStandardCategoryDetails::onNoIconSelected() {
  m_ui->m_btnIcon->setIcon(QIcon());
}

void FormStandardCategoryDetails::onLoadIconFromFile() {
  QFileDialog dialog(this, tr("Select icon file for the category"),
                     qApp->homeFolderPath(), tr("Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)"));
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setWindowIcon(qApp->icons()->fromTheme(QSL("image-generic")));
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
  m_ui->m_btnIcon->setIcon(qApp->icons()->fromTheme(QSL("folder-category")));
}

void FormStandardCategoryDetails::initialize() {
  m_ui = new Ui::FormStandardCategoryDetails();
  m_ui->setupUi(this);

  // Set text boxes.
  m_ui->m_txtTitle->lineEdit()->setPlaceholderText(tr("Category title"));
  m_ui->m_txtTitle->lineEdit()->setToolTip(tr("Set title for your category."));

  m_ui->m_txtDescription->lineEdit()->setPlaceholderText(tr("Category description"));
  m_ui->m_txtDescription->lineEdit()->setToolTip(tr("Set description for your category."));

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("folder-category")));

  // Setup button box.
  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

#if defined(Q_OS_OS2)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile = new QAction(qApp->icons()->fromTheme(QSL("image-generic")),
                                         tr("Load icon from file..."),
                                         this);
  m_actionNoIcon = new QAction(qApp->icons()->fromTheme(QSL("dialog-cancel")),
                               tr("Do not use icon"),
                               this);
  m_actionUseDefaultIcon = new QAction(qApp->icons()->fromTheme(QSL("folder-category")),
                                       tr("Use default icon"),
                                       this);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  m_iconMenu->addAction(m_actionNoIcon);
  m_ui->m_btnIcon->setMenu(m_iconMenu);

  // Setup tab order.
  setTabOrder(m_ui->m_cmbParentCategory, m_ui->m_txtTitle->lineEdit());
  setTabOrder(m_ui->m_txtTitle->lineEdit(), m_ui->m_txtDescription->lineEdit());
  setTabOrder(m_ui->m_txtDescription->lineEdit(), m_ui->m_btnIcon);
  setTabOrder(m_ui->m_btnIcon, m_ui->m_buttonBox);

  m_ui->m_txtTitle->lineEdit()->setFocus(Qt::TabFocusReason);
}

void FormStandardCategoryDetails::loadCategories(const QList<StandardCategory*> categories,
                                         RootItem *root_item,
                                         StandardCategory *input_category) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));

  foreach (StandardCategory *category, categories) {
    if (input_category != NULL && (category == input_category || category->isChildOf(input_category))) {
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
