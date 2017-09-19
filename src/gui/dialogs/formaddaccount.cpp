// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "core/feedsmodel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/standard/standardserviceentrypoint.h"

#include <QDialogButtonBox>
#include <QListWidget>

FormAddAccount::FormAddAccount(const QList<ServiceEntryPoint*>& entry_points, FeedsModel* model, QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormAddAccount), m_model(model), m_entryPoints(entry_points) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("document-new")));
  connect(m_ui->m_listEntryPoints, &QListWidget::itemDoubleClicked, this, &FormAddAccount::addSelectedAccount);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormAddAccount::addSelectedAccount);
  connect(m_ui->m_listEntryPoints, &QListWidget::itemSelectionChanged, this, &FormAddAccount::displayActiveEntryPointDetails);
  loadEntryPoints();
}

FormAddAccount::~FormAddAccount() {
  qDebug("Destroying FormAddAccount instance.");
}

void FormAddAccount::addSelectedAccount() {
  accept();
  ServiceEntryPoint* point = selectedEntryPoint();
  ServiceRoot* new_root = point->createNewRoot();

  if (new_root != nullptr) {
    m_model->addServiceAccount(new_root, true);
  }
  else {
    qCritical("Cannot create new account.");
  }
}

void FormAddAccount::displayActiveEntryPointDetails() {
  const ServiceEntryPoint* point = selectedEntryPoint();

  m_ui->m_txtAuthor->setText(point->author());
  m_ui->m_txtDescription->setText(point->description());
  m_ui->m_txtName->setText(point->name());
  m_ui->m_txtVersion->setText(point->version());
}

ServiceEntryPoint* FormAddAccount::selectedEntryPoint() const {
  return m_entryPoints.at(m_ui->m_listEntryPoints->currentRow());
}

void FormAddAccount::loadEntryPoints() {
  foreach (const ServiceEntryPoint* entry_point, m_entryPoints) {
    QListWidgetItem* item = new QListWidgetItem(entry_point->icon(), entry_point->name(), m_ui->m_listEntryPoints);

    if (entry_point->isSingleInstanceService() && m_model->containsServiceRootFromEntryPoint(entry_point)) {
      // Oops, this item cannot be added, it is single instance and is already added.
      item->setFlags(Qt::NoItemFlags);
      item->setToolTip(tr("This account can be added only once."));
    }
  }

  m_ui->m_listEntryPoints->setCurrentRow(m_entryPoints.size() - 1);
}
