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

#include "gui/dialogs/formaddaccount.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "core/feedsmodel.h"
#include "services/standard/standardserviceentrypoint.h"

#if defined(Q_OS_OS2)
#include "gui/messagebox.h"
#endif


FormAddAccount::FormAddAccount(const QList<ServiceEntryPoint*> &entry_points, FeedsModel *model, QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormAddAccount), m_model(model), m_entryPoints(entry_points) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("item-new")));

#if defined(Q_OS_OS2)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(addSelectedAccount()));
  connect(m_ui->m_listEntryPoints, SIGNAL(itemSelectionChanged()), this, SLOT(displayActiveEntryPointDetails()));
  loadEntryPoints();
}

FormAddAccount::~FormAddAccount() {
  delete m_ui;
}

void FormAddAccount::addSelectedAccount() {
  accept();

  ServiceEntryPoint *point = selectedEntryPoint();
  ServiceRoot *new_root = point->createNewRoot();

  if (new_root != NULL) {
    m_model->addServiceAccount(new_root);
  }
  else {
    qApp->showGuiMessage(tr("Cannot add account"),
                         tr("Some critical error occurred, report this to developers."),
                         QSystemTrayIcon::Critical, parentWidget(), true);
  }
}

void FormAddAccount::displayActiveEntryPointDetails() {
  ServiceEntryPoint *point = selectedEntryPoint();

  m_ui->m_txtAuthor->setText(point->author());
  m_ui->m_txtDescription->setText(point->description());
  m_ui->m_txtName->setText(point->name());
  m_ui->m_txtVersion->setText(point->version());
}

ServiceEntryPoint *FormAddAccount::selectedEntryPoint() {
  return m_entryPoints.at(m_ui->m_listEntryPoints->currentRow());
}

void FormAddAccount::loadEntryPoints() {
  foreach (ServiceEntryPoint *entry_point, m_entryPoints) {
    QListWidgetItem *item = new QListWidgetItem(entry_point->icon(), entry_point->name(), m_ui->m_listEntryPoints);

    if (entry_point->isSingleInstanceService() && m_model->containsServiceRootFromEntryPoint(entry_point)) {
      // Oops, this item cannot be added, it is single instance and is already added.
      item->setFlags(Qt::NoItemFlags);
      item->setToolTip(tr("This account can be added only once."));
    }
  }

  m_ui->m_listEntryPoints->setCurrentRow(m_entryPoints.size() - 1);
}
