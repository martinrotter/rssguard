// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formaddaccount.h"

#include "core/feedsmodel.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/standard/standardserviceentrypoint.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QSize>

FormAddAccount::FormAddAccount(const QList<ServiceEntryPoint*>& entry_points, FeedsModel* model, QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormAddAccount), m_model(model), m_entryPoints(entry_points) {
  m_ui->setupUi(this);

  GuiUtilities::applyResponsiveDialogResize(*this);

  // Set flags and attributes.
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("document-new")));

  connect(m_ui->m_listEntryPoints, &QListWidget::itemDoubleClicked, this, &FormAddAccount::addSelectedAccount);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormAddAccount::addSelectedAccount);

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

ServiceEntryPoint* FormAddAccount::selectedEntryPoint() const {
  return m_entryPoints.at(m_ui->m_listEntryPoints->currentRow());
}

void FormAddAccount::loadEntryPoints() {
  for (const ServiceEntryPoint* entry_point : m_entryPoints) {
    QListWidgetItem* item = new QListWidgetItem(entry_point->icon(), entry_point->name(), m_ui->m_listEntryPoints);

    if (entry_point->isSingleInstanceService() && m_model->containsServiceRootFromEntryPoint(entry_point)) {
      // Oops, this item cannot be added, it is single instance and is already added.
      item->setFlags(Qt::ItemFlag::NoItemFlags);
      item->setToolTip(tr("This account can be added only once."));
    }
    else {
      item->setToolTip(entry_point->description());
    }
  }

  m_ui->m_listEntryPoints->setCurrentRow(m_entryPoints.size() - 1);
}
