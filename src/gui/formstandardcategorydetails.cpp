// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/formstandardcategorydetails.h"

#include "core/defs.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/feedsmodel.h"
#include "gui/iconthemefactory.h"
#include "gui/feedsview.h"
#include "gui/baselineedit.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QFileDialog>


FormStandardCategoryDetails::FormStandardCategoryDetails(FeedsModel *model,
                                                         QWidget *parent)
  : QDialog(parent),
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
  connect(m_ui->m_buttonBox, SIGNAL(accepted()),
          this, SLOT(apply()));
  connect(m_ui->m_txtTitle->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onTitleChanged(QString)));
  connect(m_ui->m_txtDescription->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onDescriptionChanged(QString)));

  // Icon connections.
  connect(m_actionLoadIconFromFile, SIGNAL(triggered()), this, SLOT(onLoadIconFromFile()));
  connect(m_actionNoIcon, SIGNAL(triggered()), this, SLOT(onNoIconSelected()));
  connect(m_actionUseDefaultIcon, SIGNAL(triggered()), this, SLOT(onUseDefaultIcon()));
}

void FormStandardCategoryDetails::setEditableCategory(FeedsModelStandardCategory *editable_category) {
  m_editableCategory = editable_category;

  m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) editable_category->parent())));
  m_ui->m_txtTitle->lineEdit()->setText(editable_category->title());
  m_ui->m_txtDescription->lineEdit()->setText(editable_category->description());
  m_ui->m_btnIcon->setIcon(editable_category->icon());
}

int FormStandardCategoryDetails::exec(FeedsModelStandardCategory *input_category) {
  // Load categories.
  loadCategories(m_feedsModel->allCategories().values(),
                 m_feedsModel->rootItem(),
                 input_category);

  if (input_category == NULL) {
    // User is adding new category.
    setWindowTitle(tr("Add new standard category"));

    // Make sure that "default" icon is used as the default option for new
    // categories.
    m_actionUseDefaultIcon->trigger();
  }
  else {
    // User is editing existing category.
    setWindowTitle(tr("Edit existing standard category"));
    setEditableCategory(input_category);
  }

  // Run the dialog.
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
      if (SystemTrayIcon::isSystemTrayActivated()) {
        SystemTrayIcon::instance()->showMessage(tr("Cannot add category"),
                                                tr("Category was not added due to error."),
                                                QSystemTrayIcon::Critical);
      }
      else {
        MessageBox::show(this,
                         QMessageBox::Critical,
                         tr("Cannot add category"),
                         tr("Category was not added due to error."));
      }
    }
  }
  else {
    if (m_feedsModel->editStandardCategory(m_editableCategory, new_category)) {
      accept();
    }
    else {
      if (SystemTrayIcon::isSystemTrayActivated()) {
        SystemTrayIcon::instance()->showMessage(tr("Cannot edit category"),
                                                tr("Category was not edited due to error."),
                                                QSystemTrayIcon::Critical);
      }
      else {
        MessageBox::show(this,
                         QMessageBox::Critical,
                         tr("Cannot edit category"),
                         tr("Category was not edited due to error."));
      }
    }
  }
}

void FormStandardCategoryDetails::onTitleChanged(const QString &new_title){
  if (new_title.simplified().size() >= MIN_CATEGORY_NAME_LENGTH) {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Ok, tr("Category name is ok."));
  }
  else {
    m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Error, tr("Category name is too short."));
  }
}

void FormStandardCategoryDetails::onDescriptionChanged(const QString &new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Warning, tr("Description is empty."));
  }
  else {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Ok, tr("The description os ok."));
  }
}

void FormStandardCategoryDetails::onNoIconSelected() {
  m_ui->m_btnIcon->setIcon(QIcon());
}

void FormStandardCategoryDetails::onLoadIconFromFile() {
  QFileDialog dialog(this, tr("Select icon file for the category"),
                     QDir::homePath(), tr("Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)"));
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setWindowIcon(IconThemeFactory::instance()->fromTheme("image-generic"));
  dialog.setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly);
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setLabelText(QFileDialog::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));
  dialog.setLabelText(QFileDialog::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::Accepted) {
    m_ui->m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void FormStandardCategoryDetails::onUseDefaultIcon() {
  m_ui->m_btnIcon->setIcon(IconThemeFactory::instance()->fromTheme("folder-category"));
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
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::instance()->fromTheme("item-new"));

  // Setup button box.
  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

#if !defined(Q_OS_WIN)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile = new QAction(IconThemeFactory::instance()->fromTheme("image-generic"),
                                         tr("Load icon from file..."),
                                         this);
  m_actionNoIcon = new QAction(IconThemeFactory::instance()->fromTheme("dialog-cancel"),
                               tr("Do not use icon"),
                               this);
  m_actionUseDefaultIcon = new QAction(IconThemeFactory::instance()->fromTheme("folder-category"),
                                       tr("Use default icon"),
                                       this);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  m_iconMenu->addAction(m_actionNoIcon);
  m_ui->m_btnIcon->setMenu(m_iconMenu);

  // Setup tab order.
  setTabOrder(m_ui->m_buttonBox, m_ui->m_cmbParentCategory);
  setTabOrder(m_ui->m_cmbParentCategory, m_ui->m_txtTitle->lineEdit());
  setTabOrder(m_ui->m_txtTitle->lineEdit(), m_ui->m_txtDescription->lineEdit());
  setTabOrder(m_ui->m_txtDescription->lineEdit(), m_ui->m_btnIcon);
}

void FormStandardCategoryDetails::loadCategories(const QList<FeedsModelCategory*> categories,
                                                 FeedsModelRootItem *root_item,
                                                 FeedsModelCategory *input_category) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));

  foreach (FeedsModelCategory *category, categories) {
    if (input_category != NULL && (category == input_category ||
                                   category->isChildOf(input_category))) {
      // This category cannot be selected as the new
      // parent for currently edited category, so
      // don't add it.
      continue;
    }

    m_ui->m_cmbParentCategory->addItem(category->data(FDS_MODEL_TITLE_INDEX,
                                                      Qt::DecorationRole).value<QIcon>(),
                                       category->title(),
                                       QVariant::fromValue((void*) category));
  }
}
