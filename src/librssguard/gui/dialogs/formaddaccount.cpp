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
  return m_entryPoints.at(m_ui->m_listEntryPoints->currentRow());
}

void FormAddAccount::loadEntryPoints() {
  int classic_row = 0, i = 0;

  for (const ServiceEntryPoint* entry_point : qAsConst(m_entryPoints)) {
    if (entry_point->code() == QSL(SERVICE_CODE_STD_RSS)) {
      classic_row = i;
    }

    QListWidgetItem* item = new QListWidgetItem(entry_point->icon(), entry_point->name(), m_ui->m_listEntryPoints);

    item->setToolTip(entry_point->description());
    i++;
  }

  m_ui->m_listEntryPoints->setCurrentRow(classic_row);
}
