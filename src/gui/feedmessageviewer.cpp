// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/feedmessageviewer.h"

#include "miscellaneous/settings.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/databasecleaner.h"
#include "core/messagesproxymodel.h"
#include "core/feeddownloader.h"
#include "core/feedsproxymodel.h"
#include "services/standard/standardserviceroot.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardfeedsimportexportmodel.h"
#include "gui/messagesview.h"
#include "gui/feedsview.h"
#include "gui/statusbar.h"
#include "gui/systemtrayicon.h"
#include "gui/messagebox.h"
#include "gui/messagestoolbar.h"
#include "gui/feedstoolbar.h"
#include "gui/messagepreviewer.h"
#include "gui/dialogs/formdatabasecleanup.h"
#include "gui/dialogs/formmain.h"
#include "exceptions/applicationexception.h"

#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QDebug>
#include <QLineEdit>
#include <QAction>
#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>
#include <QThread>
#include <QProgressBar>
#include <QStatusBar>
#include <QPointer>


FeedMessageViewer::FeedMessageViewer(QWidget *parent)
  : TabContent(parent),
    m_toolBarsEnabled(true),
    m_listHeadersEnabled(true),
    m_toolBarFeeds(new FeedsToolBar(tr("Toolbar for feeds"), this)),
    m_toolBarMessages(new MessagesToolBar(tr("Toolbar for messages"), this)),
    m_messagesView(new MessagesView(this)),
    m_feedsView(new FeedsView(this)),
    m_messagesBrowser(new MessagePreviewer(this)) {
  initialize();
  initializeViews();
  loadMessageViewerFonts();
  createConnections();
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebug("Destroying FeedMessageViewer instance.");
}

void FeedMessageViewer::saveSize() {
  Settings *settings = qApp->settings();
  
  m_feedsView->saveAllExpandStates();
  
  // Store offsets of splitters.
  settings->setValue(GROUP(GUI), GUI::SplitterFeeds, QString(m_feedSplitter->saveState().toBase64()));
  settings->setValue(GROUP(GUI), GUI::SplitterMessages, QString(m_messageSplitter->saveState().toBase64()));
  
  // States of splitters are stored, let's store
  // widths of columns.
  int width_column_author = m_messagesView->columnWidth(MSG_DB_AUTHOR_INDEX);
  int width_column_date = m_messagesView->columnWidth(MSG_DB_DCREATED_INDEX);
  
  if (width_column_author != 0 && width_column_date != 0) {
    settings->setValue(GROUP(GUI), KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX), width_column_author);
    settings->setValue(GROUP(GUI), KEY_MESSAGES_VIEW + QString::number(MSG_DB_DCREATED_INDEX), width_column_date);
  }
  
  // Store "visibility" of toolbars and list headers.
  settings->setValue(GROUP(GUI), GUI::ToolbarsVisible, m_toolBarsEnabled);
  settings->setValue(GROUP(GUI), GUI::ListHeadersVisible, m_listHeadersEnabled);
}

void FeedMessageViewer::loadSize() {
  const Settings *settings = qApp->settings();
  const int default_msg_section_size = m_messagesView->header()->defaultSectionSize();
  
  // Restore offsets of splitters.
  m_feedSplitter->restoreState(QByteArray::fromBase64(settings->value(GROUP(GUI), SETTING(GUI::SplitterFeeds)).toString().toLocal8Bit()));
  m_messageSplitter->restoreState(QByteArray::fromBase64(settings->value(GROUP(GUI), SETTING(GUI::SplitterMessages)).toString().toLocal8Bit()));
  
  // Splitters are restored, now, restore widths of columns.
  m_messagesView->setColumnWidth(MSG_DB_AUTHOR_INDEX, settings->value(GROUP(GUI),
                                                                      KEY_MESSAGES_VIEW + QString::number(MSG_DB_AUTHOR_INDEX),
                                                                      default_msg_section_size).toInt());
  m_messagesView->setColumnWidth(MSG_DB_DCREATED_INDEX, settings->value(GROUP(GUI),
                                                                        KEY_MESSAGES_VIEW + QString::number(MSG_DB_DCREATED_INDEX),
                                                                        default_msg_section_size).toInt());
}

void FeedMessageViewer::loadMessageViewerFonts() {
  const Settings *settings = qApp->settings();

  // TODO: TODO
  //QWebEngineSettings *view_settings = m_messagesBrowser->view()->settings();
  
  //view_settings->setFontFamily(QWebEngineSettings::StandardFont, settings->value(GROUP(Messages),
  //                                                                         SETTING(Messages::PreviewerFontStandard)).toString());
}

void FeedMessageViewer::quit() {
  // Quit the feeds model (stops auto-update timer etc.).
  m_feedsView->sourceModel()->quit();
}

bool FeedMessageViewer::areToolBarsEnabled() const {
  return m_toolBarsEnabled;
}

bool FeedMessageViewer::areListHeadersEnabled() const {
  return m_listHeadersEnabled;
}

void FeedMessageViewer::switchMessageSplitterOrientation() {
  if (m_messageSplitter->orientation() == Qt::Vertical) {
    m_messageSplitter->setOrientation(Qt::Horizontal);
  }
  else {
    m_messageSplitter->setOrientation(Qt::Vertical);
  }
}

void FeedMessageViewer::setToolBarsEnabled(bool enable) {
  m_toolBarsEnabled = enable;
  m_toolBarFeeds->setVisible(enable);
  m_toolBarMessages->setVisible(enable);
}

void FeedMessageViewer::setListHeadersEnabled(bool enable) {
  m_listHeadersEnabled = enable;
  m_feedsView->header()->setVisible(enable);
  m_messagesView->header()->setVisible(enable);
}

void FeedMessageViewer::switchFeedComponentVisibility() {
  QAction *sen = qobject_cast<QAction*>(sender());

  if (sen != NULL) {
    m_feedsWidget->setVisible(sen->isChecked());
  }
  else {
    m_feedsWidget->setVisible(!m_feedsWidget->isVisible());
  }
}

void FeedMessageViewer::toggleShowOnlyUnreadFeeds() {
  const QAction *origin = qobject_cast<QAction*>(sender());
  
  if (origin == NULL) {
    m_feedsView->model()->invalidateReadFeedsFilter(true, false);
  }
  else {
    m_feedsView->model()->invalidateReadFeedsFilter(true, origin->isChecked());
  }
}

void FeedMessageViewer::updateMessageButtonsAvailability() {
  const bool one_message_selected = m_messagesView->selectionModel()->selectedRows().size() == 1;
  const bool atleast_one_message_selected = !m_messagesView->selectionModel()->selectedRows().isEmpty();
  const bool bin_loaded = m_messagesView->sourceModel()->loadedItem() != NULL && m_messagesView->sourceModel()->loadedItem()->kind() == RootItemKind::Bin;
  const FormMain *form_main = qApp->mainForm();
  
  form_main->m_ui->m_actionDeleteSelectedMessages->setEnabled(atleast_one_message_selected);
  form_main->m_ui->m_actionRestoreSelectedMessages->setEnabled(atleast_one_message_selected && bin_loaded);
  form_main->m_ui->m_actionMarkSelectedMessagesAsRead->setEnabled(atleast_one_message_selected);
  form_main->m_ui->m_actionMarkSelectedMessagesAsUnread->setEnabled(atleast_one_message_selected);
  form_main->m_ui->m_actionOpenSelectedMessagesInternally->setEnabled(atleast_one_message_selected);
  form_main->m_ui->m_actionOpenSelectedSourceArticlesExternally->setEnabled(atleast_one_message_selected);
  form_main->m_ui->m_actionSendMessageViaEmail->setEnabled(one_message_selected);
  form_main->m_ui->m_actionSwitchImportanceOfSelectedMessages->setEnabled(atleast_one_message_selected);
}

void FeedMessageViewer::updateFeedButtonsAvailability() {
  const bool is_update_running = feedsView()->sourceModel()->isFeedUpdateRunning();
  const bool critical_action_running = qApp->feedUpdateLock()->isLocked();
  const RootItem *selected_item = feedsView()->selectedItem();
  const bool anything_selected = selected_item != NULL;
  const bool feed_selected = anything_selected && selected_item->kind() == RootItemKind::Feed;
  const bool category_selected = anything_selected && selected_item->kind() == RootItemKind::Category;
  const bool service_selected = anything_selected && selected_item->kind() == RootItemKind::ServiceRoot;
  const FormMain *form_main = qApp->mainForm();
  
  form_main->m_ui->m_actionStopRunningItemsUpdate->setEnabled(is_update_running);
  form_main->m_ui->m_actionBackupDatabaseSettings->setEnabled(!critical_action_running);
  form_main->m_ui->m_actionCleanupDatabase->setEnabled(!critical_action_running);
  form_main->m_ui->m_actionClearSelectedItems->setEnabled(anything_selected);
  form_main->m_ui->m_actionDeleteSelectedItem->setEnabled(!critical_action_running && anything_selected);
  form_main->m_ui->m_actionEditSelectedItem->setEnabled(!critical_action_running && anything_selected);
  form_main->m_ui->m_actionMarkSelectedItemsAsRead->setEnabled(anything_selected);
  form_main->m_ui->m_actionMarkSelectedItemsAsUnread->setEnabled(anything_selected);
  form_main->m_ui->m_actionUpdateAllItems->setEnabled(!critical_action_running);
  form_main->m_ui->m_actionUpdateSelectedItems->setEnabled(!critical_action_running && (feed_selected || category_selected || service_selected));
  form_main->m_ui->m_actionViewSelectedItemsNewspaperMode->setEnabled(anything_selected);
  form_main->m_ui->m_actionExpandCollapseItem->setEnabled(anything_selected);

  form_main->m_ui->m_actionServiceDelete->setEnabled(service_selected);
  form_main->m_ui->m_actionServiceEdit->setEnabled(service_selected);
  form_main->m_ui->m_actionAddFeedIntoSelectedAccount->setEnabled(anything_selected);
  form_main->m_ui->m_actionAddCategoryIntoSelectedAccount->setEnabled(anything_selected);

  form_main->m_ui->m_menuAddItem->setEnabled(!critical_action_running);
  form_main->m_ui->m_menuAccounts->setEnabled(!critical_action_running);
  form_main->m_ui->m_menuRecycleBin->setEnabled(!critical_action_running);
}

void FeedMessageViewer::createConnections() {
  const FormMain *form_main = qApp->mainForm();
  
  // Filtering & searching.
  connect(m_toolBarMessages, SIGNAL(messageSearchPatternChanged(QString)), m_messagesView, SLOT(searchMessages(QString)));
  connect(m_toolBarMessages, SIGNAL(messageFilterChanged(MessagesModel::MessageHighlighter)), m_messagesView, SLOT(filterMessages(MessagesModel::MessageHighlighter)));
  
  // Message changers.
  connect(m_messagesView, SIGNAL(currentMessageRemoved()), m_messagesBrowser, SLOT(clear()));
  connect(m_messagesView, SIGNAL(currentMessageChanged(Message,RootItem*)), m_messagesBrowser, SLOT(loadMessage(Message,RootItem*)));
  connect(m_messagesView, SIGNAL(currentMessageRemoved()), this, SLOT(updateMessageButtonsAvailability()));
  connect(m_messagesView, SIGNAL(currentMessageChanged(Message,RootItem*)), this, SLOT(updateMessageButtonsAvailability()));
  connect(m_messagesBrowser, SIGNAL(requestMessageListReload(bool)), m_messagesView, SLOT(reloadSelections(bool)));


  connect(m_feedsView, SIGNAL(itemSelected(RootItem*)), this, SLOT(updateFeedButtonsAvailability()));
  connect(qApp->feedUpdateLock(), SIGNAL(locked()), this, SLOT(updateFeedButtonsAvailability()));
  connect(qApp->feedUpdateLock(), SIGNAL(unlocked()), this, SLOT(updateFeedButtonsAvailability()));
  
  // If user selects feeds, load their messages.
  connect(m_feedsView, SIGNAL(itemSelected(RootItem*)), m_messagesView, SLOT(loadItem(RootItem*)));
  
  // State of many messages is changed, then we need
  // to reload selections.
  connect(m_feedsView->sourceModel(), SIGNAL(reloadMessageListRequested(bool)),
          m_messagesView, SLOT(reloadSelections(bool)));
  connect(m_feedsView->sourceModel(), SIGNAL(feedsUpdateFinished()), this, SLOT(onFeedsUpdateFinished()));
  connect(m_feedsView->sourceModel(), SIGNAL(feedsUpdateStarted()), this, SLOT(onFeedsUpdateStarted()));

  // Message openers.
  connect(m_messagesView, SIGNAL(openMessagesInNewspaperView(QList<Message>)),
          form_main->m_ui->m_tabWidget, SLOT(addBrowserWithMessages(QList<Message>)));
  connect(m_feedsView, SIGNAL(openMessagesInNewspaperView(QList<Message>)),
          form_main->m_ui->m_tabWidget, SLOT(addBrowserWithMessages(QList<Message>)));
  
  // Toolbar forwardings.
  connect(form_main->m_ui->m_actionAddFeedIntoSelectedAccount, SIGNAL(triggered()),
          m_feedsView, SLOT(addFeedIntoSelectedAccount()));
  connect(form_main->m_ui->m_actionAddCategoryIntoSelectedAccount, SIGNAL(triggered()),
          m_feedsView, SLOT(addCategoryIntoSelectedAccount()));
  connect(form_main->m_ui->m_actionCleanupDatabase,
          SIGNAL(triggered()), this, SLOT(showDbCleanupAssistant()));
  connect(form_main->m_ui->m_actionSwitchImportanceOfSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(switchSelectedMessagesImportance()));
  connect(form_main->m_ui->m_actionDeleteSelectedMessages,
          SIGNAL(triggered()), m_messagesView, SLOT(deleteSelectedMessages()));
  connect(form_main->m_ui->m_actionMarkSelectedMessagesAsRead,
          SIGNAL(triggered()), m_messagesView, SLOT(markSelectedMessagesRead()));
  connect(form_main->m_ui->m_actionMarkSelectedMessagesAsUnread,
          SIGNAL(triggered()), m_messagesView, SLOT(markSelectedMessagesUnread()));
  connect(form_main->m_ui->m_actionOpenSelectedSourceArticlesExternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedSourceMessagesExternally()));
  connect(form_main->m_ui->m_actionOpenSelectedMessagesInternally,
          SIGNAL(triggered()), m_messagesView, SLOT(openSelectedMessagesInternally()));
  connect(form_main->m_ui->m_actionSendMessageViaEmail,
          SIGNAL(triggered()), m_messagesView, SLOT(sendSelectedMessageViaEmail()));
  connect(form_main->m_ui->m_actionMarkAllItemsRead,
          SIGNAL(triggered()), m_feedsView, SLOT(markAllItemsRead()));
  connect(form_main->m_ui->m_actionMarkSelectedItemsAsRead,
          SIGNAL(triggered()), m_feedsView, SLOT(markSelectedItemRead()));
  connect(form_main->m_ui->m_actionExpandCollapseItem,
          SIGNAL(triggered()), m_feedsView, SLOT(expandCollapseCurrentItem()));
  connect(form_main->m_ui->m_actionMarkSelectedItemsAsUnread,
          SIGNAL(triggered()), m_feedsView, SLOT(markSelectedItemUnread()));
  connect(form_main->m_ui->m_actionClearSelectedItems,
          SIGNAL(triggered()), m_feedsView, SLOT(clearSelectedFeeds()));
  connect(form_main->m_ui->m_actionClearAllItems,
          SIGNAL(triggered()), m_feedsView, SLOT(clearAllFeeds()));
  connect(form_main->m_ui->m_actionUpdateSelectedItems,
          SIGNAL(triggered()), m_feedsView, SLOT(updateSelectedItems()));
  connect(form_main->m_ui->m_actionUpdateAllItems,
          SIGNAL(triggered()), m_feedsView, SLOT(updateAllItems()));
  connect(form_main->m_ui->m_actionStopRunningItemsUpdate,
          SIGNAL(triggered()), m_feedsView->sourceModel(), SLOT(stopRunningFeedUpdate()));
  connect(form_main->m_ui->m_actionEditSelectedItem,
          SIGNAL(triggered()), m_feedsView, SLOT(editSelectedItem()));
  connect(form_main->m_ui->m_actionViewSelectedItemsNewspaperMode,
          SIGNAL(triggered()), m_feedsView, SLOT(openSelectedItemsInNewspaperMode()));
  connect(form_main->m_ui->m_actionDeleteSelectedItem,
          SIGNAL(triggered()), m_feedsView, SLOT(deleteSelectedItem()));
  connect(form_main->m_ui->m_actionSwitchFeedsList,
          SIGNAL(triggered()), this, SLOT(switchFeedComponentVisibility()));
  connect(form_main->m_ui->m_actionSelectNextItem,
          SIGNAL(triggered()), m_feedsView, SLOT(selectNextItem()));
  connect(form_main->m_ui->m_actionSwitchToolBars,
          SIGNAL(toggled(bool)), this, SLOT(setToolBarsEnabled(bool)));
  connect(form_main->m_ui->m_actionSwitchListHeaders,
          SIGNAL(toggled(bool)), this, SLOT(setListHeadersEnabled(bool)));
  connect(form_main->m_ui->m_actionSelectPreviousItem,
          SIGNAL(triggered()), m_feedsView, SLOT(selectPreviousItem()));
  connect(form_main->m_ui->m_actionSelectNextMessage,
          SIGNAL(triggered()), m_messagesView, SLOT(selectNextItem()));
  connect(form_main->m_ui->m_actionSelectNextUnreadMessage,
          SIGNAL(triggered()), m_messagesView, SLOT(selectNextUnreadItem()));
  connect(form_main->m_ui->m_actionSelectPreviousMessage,
          SIGNAL(triggered()), m_messagesView, SLOT(selectPreviousItem()));
  connect(form_main->m_ui->m_actionSwitchMessageListOrientation, SIGNAL(triggered()),
          this, SLOT(switchMessageSplitterOrientation()));
  connect(form_main->m_ui->m_actionShowOnlyUnreadItems, SIGNAL(toggled(bool)),
          this, SLOT(toggleShowOnlyUnreadFeeds()));
  connect(form_main->m_ui->m_actionRestoreSelectedMessages, SIGNAL(triggered()),
          m_messagesView, SLOT(restoreSelectedMessages()));
  connect(form_main->m_ui->m_actionRestoreAllRecycleBins, SIGNAL(triggered()),
          m_feedsView->sourceModel(), SLOT(restoreAllBins()));
  connect(form_main->m_ui->m_actionEmptyAllRecycleBins, SIGNAL(triggered()),
          m_feedsView->sourceModel(), SLOT(emptyAllBins()));
}

void FeedMessageViewer::initialize() {
  // Initialize/populate toolbars.
  m_toolBarFeeds->setFloatable(false);
  m_toolBarFeeds->setMovable(false);
  m_toolBarFeeds->setAllowedAreas(Qt::TopToolBarArea);
  m_toolBarFeeds->loadChangeableActions();
  
  m_toolBarMessages->setFloatable(false);
  m_toolBarMessages->setMovable(false);
  m_toolBarMessages->setAllowedAreas(Qt::TopToolBarArea);
  m_toolBarMessages->loadChangeableActions();
   
  // Now refresh visual setup.
  refreshVisualProperties();
}

void FeedMessageViewer::initializeViews() {
  m_feedsWidget = new QWidget(this);
  m_messagesWidget = new QWidget(this);
  m_feedSplitter = new QSplitter(Qt::Horizontal, this);
  m_messageSplitter = new QSplitter(Qt::Vertical, this);
  
  // Instantiate needed components.
  QVBoxLayout *central_layout = new QVBoxLayout(this);
  QVBoxLayout *feed_layout = new QVBoxLayout(m_feedsWidget);
  QVBoxLayout *message_layout = new QVBoxLayout(m_messagesWidget);
  
  // Set layout properties.
  central_layout->setMargin(0);
  central_layout->setSpacing(0);
  feed_layout->setMargin(0);
  feed_layout->setSpacing(0);
  message_layout->setMargin(0);
  message_layout->setSpacing(0);
  
  // Set views.
  m_feedsView->setFrameStyle(QFrame::NoFrame);
  m_messagesView->setFrameStyle(QFrame::NoFrame);
  
  // Setup message splitter.
  m_messageSplitter->setObjectName(QSL("MessageSplitter"));
  m_messageSplitter->setHandleWidth(1);
  m_messageSplitter->setOpaqueResize(false);
  m_messageSplitter->setChildrenCollapsible(false);
  m_messageSplitter->addWidget(m_messagesView);
  m_messageSplitter->addWidget(m_messagesBrowser);
  
  // Assemble message-related components to single widget.
  message_layout->addWidget(m_toolBarMessages);
  message_layout->addWidget(m_messageSplitter);
  
  // Assemble feed-related components to another widget.
  feed_layout->addWidget(m_toolBarFeeds);
  feed_layout->addWidget(m_feedsView);
  
  // Assembler everything together.
  m_feedSplitter->setHandleWidth(1);
  m_feedSplitter->setOpaqueResize(false);
  m_feedSplitter->setChildrenCollapsible(false);
  m_feedSplitter->addWidget(m_feedsWidget);
  m_feedSplitter->addWidget(m_messagesWidget);
  
  // Add toolbar and main feeds/messages widget to main layout.
  central_layout->addWidget(m_feedSplitter);
  
  setTabOrder(m_feedsView, m_messagesView);
  setTabOrder(m_messagesView, m_toolBarFeeds);
  setTabOrder(m_toolBarFeeds, m_toolBarMessages);
  setTabOrder(m_toolBarMessages, m_messagesBrowser);
  
  updateMessageButtonsAvailability();
  updateFeedButtonsAvailability();
}

void FeedMessageViewer::showDbCleanupAssistant() {
  if (qApp->feedUpdateLock()->tryLock()) {
    QScopedPointer<FormDatabaseCleanup> form_pointer(new FormDatabaseCleanup(this));
    form_pointer.data()->setCleaner(m_feedsView->sourceModel()->databaseCleaner());
    form_pointer.data()->exec();
    
    qApp->feedUpdateLock()->unlock();
    
    m_messagesView->reloadSelections(false);
    m_feedsView->sourceModel()->reloadCountsOfWholeModel();
  }
  else {
    qApp->showGuiMessage(tr("Cannot cleanup database"),
                         tr("Cannot cleanup database, because another critical action is running."),
                         QSystemTrayIcon::Warning, qApp->mainForm(), true);
  }
}

void FeedMessageViewer::refreshVisualProperties() {
  const Qt::ToolButtonStyle button_style = static_cast<Qt::ToolButtonStyle>(qApp->settings()->value(GROUP(GUI),
                                                                                                    SETTING(GUI::ToolbarStyle)).toInt());
  
  m_toolBarFeeds->setToolButtonStyle(button_style);
  m_toolBarMessages->setToolButtonStyle(button_style);
}

void FeedMessageViewer::onFeedsUpdateFinished() {
  m_messagesView->reloadSelections(true);
}

void FeedMessageViewer::onFeedsUpdateStarted() {
  // Check only "Stop running update" button.
  const bool is_update_running = feedsView()->sourceModel()->isFeedUpdateRunning();

  qApp->mainForm()->m_ui->m_actionStopRunningItemsUpdate->setEnabled(is_update_running);
}
