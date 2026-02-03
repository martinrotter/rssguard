// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formaddaccount.h"

#include "core/feedsmodel.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceentrypoint.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QSize>

FormAddAccount::FormAddAccount(const QList<ServiceEntryPoint*>& entry_points, FeedsModel* model, QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormAddAccount), m_model(model), m_entryPoints(entry_points) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("list-add")));

  connect(m_ui->m_listEntryPoints, &QListWidget::itemDoubleClicked, this, &FormAddAccount::addSelectedAccount);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormAddAccount::addSelectedAccount);
  connect(m_ui->m_listEntryPoints, &QListWidget::currentRowChanged, this, &FormAddAccount::showAccountDetails);

  loadEntryPoints();
}

FormAddAccount::~FormAddAccount() {
  qDebugNN << LOGSEC_GUI << "Destroying FormAddAccount instance.";
}

void FormAddAccount::addSelectedAccount() {
  accept();
  ServiceEntryPoint* point = selectedEntryPoint();

  if (point == nullptr) {
    return;
  }

  ServiceRoot* new_root = point->createNewRoot();

  if (new_root != nullptr) {
    m_model->addServiceAccount(new_root, true);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Cannot create new account.";
  }
}

void FormAddAccount::showAccountDetails() {
  ServiceEntryPoint* point = selectedEntryPoint();

  if (point != nullptr) {
    m_ui->m_lblDetails->setText(point->description());
  }
}

ServiceEntryPoint* FormAddAccount::selectedEntryPoint() const {
  auto* itm = m_ui->m_listEntryPoints->currentItem();

  if (itm == nullptr) {
    return nullptr;
  }
  else {
    return reinterpret_cast<ServiceEntryPoint*>(itm->data(Qt::ItemDataRole::UserRole).value<intptr_t>());
  }
}

void FormAddAccount::loadEntryPoints() {
  for (const ServiceEntryPoint* entry_point : std::as_const(m_entryPoints)) {
    QListWidgetItem* item = new QListWidgetItem(entry_point->icon(), entry_point->name());

    item->setToolTip(entry_point->description());
    item->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue<intptr_t>(reinterpret_cast<intptr_t>(entry_point)));

    if (entry_point->code() == QSL(SERVICE_CODE_STD_RSS)) {
      m_ui->m_listEntryPoints->insertItem(0, item);
      m_ui->m_listEntryPoints->insertItem(1, QSL("--------"));

      m_ui->m_listEntryPoints->item(1)->setFlags(Qt::ItemFlag::NoItemFlags);
    }
    else {
      m_ui->m_listEntryPoints->addItem(item);
    }
  }

  m_ui->m_listEntryPoints->setCurrentRow(0);
}
