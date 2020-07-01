// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmessagefiltersmanager.h"

#include "core/messagefilter.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/accountcheckmodel.h"

FormMessageFiltersManager::FormMessageFiltersManager(FeedReader* reader, const QList<ServiceRoot*>& accounts, QWidget* parent)
  : QDialog(parent), m_feedsModel(new AccountCheckModel(this)), m_rootItem(new RootItem()),
  m_accounts(accounts), m_reader(reader) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("view-list-details")));

  m_ui.m_btnAddNew->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnRemoveSelected->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));
  m_ui.m_btnTest->setIcon(qApp->icons()->fromTheme(QSL("media-playback-start")));
  m_ui.m_btnDetailedHelp->setIcon(qApp->icons()->fromTheme(QSL("help-contents")));
  m_ui.m_txtScript->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));

  connect(m_ui.m_btnDetailedHelp, &QPushButton::clicked, this, []() {
    qApp->web()->openUrlInExternalBrowser(MSG_FILTERING_HELP);
  });
  connect(m_ui.m_listFilters, &QListWidget::currentRowChanged,
          this, &FormMessageFiltersManager::loadFilter);
  connect(m_ui.m_btnAddNew, &QPushButton::clicked,
          this, &FormMessageFiltersManager::addNewFilter);
}

FormMessageFiltersManager::~FormMessageFiltersManager() {
  delete m_rootItem;
}

MessageFilter* FormMessageFiltersManager::selectedFilter() const {
  if (m_ui.m_listFilters->selectedItems().isEmpty()) {
    return nullptr;
  }
  else {
    return m_ui.m_listFilters->selectedItems().at(0)->data(Qt::ItemDataRole::UserRole).value<MessageFilter*>();
  }
}

ServiceRoot* FormMessageFiltersManager::selectedAccount() const {
  return nullptr;
}

void FormMessageFiltersManager::addNewFilter() {
  auto* fltr = m_reader->addMessageFilter(
    tr("New message filter"),
    QSL("function filterMessage() { return 1; }"));
  auto* it = new QListWidgetItem(fltr->name(), m_ui.m_listFilters);

  it->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue<MessageFilter*>(fltr));

  m_ui.m_listFilters->setCurrentRow(m_ui.m_listFilters->count() - 1);
}

void FormMessageFiltersManager::loadFilter() {
  auto* filter = selectedFilter();
  auto* acc = selectedAccount();

  showFilter(filter);
  updateFeedAssignments(filter, acc);
}

void FormMessageFiltersManager::showFilter(MessageFilter* filter) {
  if (filter == nullptr) {
    m_ui.m_txtTitle->clear();
    m_ui.m_txtScript->clear();
    m_ui.m_gbDetails->setEnabled(false);
  }
  else {
    m_ui.m_txtTitle->setText(filter->name());
    m_ui.m_txtScript->setPlainText(filter->script());
    m_ui.m_gbDetails->setEnabled(true);
  }
}

void FormMessageFiltersManager::updateFeedAssignments(MessageFilter* filter, ServiceRoot* account) {}
