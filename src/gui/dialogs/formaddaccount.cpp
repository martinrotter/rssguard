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

  connect(m_ui->m_listEntryPoints, SIGNAL(itemSelectionChanged()), this, SLOT(displayActiveEntryPointDetails()));
  loadEntryPoints();
}

FormAddAccount::~FormAddAccount() {
  delete m_ui;
}

void FormAddAccount::displayActiveEntryPointDetails() {
  QList<QListWidgetItem*> selected_items = m_ui->m_listEntryPoints->selectedItems();

  if (!selected_items.isEmpty()) {
    ServiceEntryPoint *point = static_cast<ServiceEntryPoint*>(selected_items.at(0)->data(Qt::UserRole).value<void*>());
  }
}

void FormAddAccount::loadEntryPoints() {
  foreach (ServiceEntryPoint *entry_point, m_entryPoints) {
    QListWidgetItem *item = new QListWidgetItem(entry_point->icon(), entry_point->name(), m_ui->m_listEntryPoints);

    item->setData(Qt::UserRole, QVariant::fromValue((void*) entry_point));

  }
}
