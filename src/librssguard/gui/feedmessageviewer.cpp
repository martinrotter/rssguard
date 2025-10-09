// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/feedmessageviewer.h"

#include "core/feedsproxymodel.h"
#include "core/messagesproxymodel.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedsview.h"
#include "gui/messagepreviewer.h"
#include "gui/messagesview.h"
#include "gui/toolbars/feedstoolbar.h"
#include "gui/toolbars/messagestoolbar.h"
#include "gui/webbrowser.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/templates.h"

#include <QAction>
#include <QDebug>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QProgressBar>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

FeedMessageViewer::FeedMessageViewer(QWidget* parent)
  : TabContent(parent), m_toolBarsEnabled(true), m_listHeadersEnabled(true),
    m_toolBarFeeds(new FeedsToolBar(tr("Toolbar for feeds"), this)),
    m_toolBarMessages(new MessagesToolBar(tr("Toolbar for articles"), this)), m_messagesView(new MessagesView(this)),
    m_feedsView(new FeedsView(this)), m_messagesBrowser(new MessagePreviewer(this)) {
  initialize();
  initializeViews();
  createConnections();
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebugNN << LOGSEC_GUI << "Destroying FeedMessageViewer instance.";
}

WebBrowser* FeedMessageViewer::webBrowser() const {
  return m_messagesBrowser->webBrowser();
}

FeedsView* FeedMessageViewer::feedsView() const {
  return m_feedsView;
}

MessagesView* FeedMessageViewer::messagesView() const {
  return m_messagesView;
}

MessagesToolBar* FeedMessageViewer::messagesToolBar() const {
  return m_toolBarMessages;
}

FeedsToolBar* FeedMessageViewer::feedsToolBar() const {
  return m_toolBarFeeds;
}

void FeedMessageViewer::saveSize() {
  Settings* settings = qApp->settings();

  settings->setValue(GROUP(GUI), GUI::FeedViewState, QString(m_feedsView->saveHeaderState().toBase64()));
  settings->setValue(GROUP(GUI), GUI::MessageViewState, QString(m_messagesView->saveHeaderState().toBase64()));

  // Store "visibility" of toolbars and list headers.
  settings->setValue(GROUP(GUI), GUI::ToolbarsVisible, m_toolBarsEnabled);
  settings->setValue(GROUP(GUI), GUI::ListHeadersVisible, m_listHeadersEnabled);
}

void FeedMessageViewer::loadSize() {
  const Settings* settings = qApp->settings();

  // Restore offsets of splitters.
  m_feedSplitter->setSizes(toList<int>(settings->value(GROUP(GUI), SETTING(GUI::SplitterFeeds))));

  if (settings->value(GROUP(GUI), SETTING(GUI::SplitterMessagesIsVertical)).toBool()) {
    m_messageSplitter->setSizes(toList<int>(settings->value(GROUP(GUI), SETTING(GUI::SplitterMessagesVertical))));
  }
  else {
    switchMessageSplitterOrientation();
  }

  QString settings_feed_header = settings->value(GROUP(GUI), SETTING(GUI::FeedViewState)).toString();

  if (!settings_feed_header.isEmpty()) {
    m_feedsView->restoreHeaderState(QByteArray::fromBase64(settings_feed_header.toLocal8Bit()));
  }

  QString settings_msg_header = settings->value(GROUP(GUI), SETTING(GUI::MessageViewState)).toString();

  if (!settings_msg_header.isEmpty()) {
    m_messagesView->restoreHeaderState(QByteArray::fromBase64(settings_msg_header.toLocal8Bit()));
  }
  else {
    // Set default sort column.
    m_messagesView->header()->setSortIndicator(MSG_DB_DCREATED_INDEX, Qt::SortOrder::DescendingOrder);
  }
}

void FeedMessageViewer::loadMessageViewerFonts() {
  m_messagesBrowser->reloadFontSettings();
  m_messagesView->reloadFontSettings();
  m_feedsView->reloadFontSettings();
}

bool FeedMessageViewer::areToolBarsEnabled() const {
  return m_toolBarsEnabled;
}

bool FeedMessageViewer::areListHeadersEnabled() const {
  return m_listHeadersEnabled;
}

void FeedMessageViewer::normalizeToolbarHeights() {
  auto max_height = std::max(m_toolBarFeeds->height(), m_toolBarMessages->height());

  m_toolBarFeeds->setMinimumHeight(max_height);
  m_toolBarMessages->setMinimumHeight(max_height);
}

void FeedMessageViewer::onFeedSplitterResized() {
  qDebugNN << LOGSEC_GUI << "Feed splitter moved.";

  qApp->settings()->setValue(GROUP(GUI), GUI::SplitterFeeds, toVariant(m_feedSplitter->sizes()));
}

void FeedMessageViewer::onMessageSplitterResized() {
  qDebugNN << LOGSEC_GUI << "Message splitter moved.";

  QList<int> sizes = m_messageSplitter->sizes();

  if (sizes.size() == 2 && (sizes[0] == 0 || sizes[1] == 0)) {
    qWarningNN << LOGSEC_GUI << "Some of splitter position is 0.";
    return;
  }

  QVariant siz = toVariant(sizes);

  if (m_messageSplitter->orientation() == Qt::Orientation::Vertical) {
    qApp->settings()->setValue(GROUP(GUI), GUI::SplitterMessagesVertical, siz);
  }
  else {
    qApp->settings()->setValue(GROUP(GUI), GUI::SplitterMessagesHorizontal, siz);
  }
}

void FeedMessageViewer::switchMessageSplitterOrientation() {
  if (m_messageSplitter->orientation() == Qt::Orientation::Vertical) {
    m_messageSplitter->setOrientation(Qt::Orientation::Horizontal);
    m_messageSplitter->setSizes(toList<int>(qApp->settings()->value(GROUP(GUI),
                                                                    SETTING(GUI::SplitterMessagesHorizontal))));
  }
  else {
    m_messageSplitter->setOrientation(Qt::Orientation::Vertical);
    m_messageSplitter->setSizes(toList<int>(qApp->settings()->value(GROUP(GUI),
                                                                    SETTING(GUI::SplitterMessagesVertical))));
  }

  qApp->settings()->setValue(GROUP(GUI),
                             GUI::SplitterMessagesIsVertical,
                             m_messageSplitter->orientation() == Qt::Orientation::Vertical);
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
  auto* sen = qobject_cast<QAction*>(sender());

  if (sen != nullptr) {
    m_feedsWidget->setVisible(sen->isChecked());
  }
  else {
    m_feedsWidget->setVisible(!m_feedsWidget->isVisible());
  }
}

void FeedMessageViewer::changeMessageFilter(MessagesProxyModel::MessageListFilter filter) {
  m_messagesView->changeFilter(filter);
}

void FeedMessageViewer::changeFeedFilter(FeedsProxyModel::FeedListFilter filter) {
  m_feedsView->changeFilter(filter);
}

void FeedMessageViewer::toggleShowFeedTreeBranches() {
  const QAction* origin = qobject_cast<QAction*>(sender());

  m_feedsView->setRootIsDecorated(origin->isChecked());
  qApp->settings()->setValue(GROUP(Feeds), Feeds::ShowTreeBranches, origin->isChecked());
}

void FeedMessageViewer::toggleItemsAutoExpandingOnSelection() {
  const QAction* origin = qobject_cast<QAction*>(sender());

  qApp->settings()->setValue(GROUP(Feeds), Feeds::AutoExpandOnSelection, origin->isChecked());
}

void FeedMessageViewer::alternateRowColorsInLists() {
  const QAction* origin = qobject_cast<QAction*>(sender());

  m_feedsView->setAlternatingRowColors(origin->isChecked());
  m_messagesView->setAlternatingRowColors(origin->isChecked());

  qApp->settings()->setValue(GROUP(GUI), GUI::AlternateRowColorsInLists, origin->isChecked());
}

void FeedMessageViewer::respondToMainWindowResizes() {
  connect(qApp->mainForm(), &FormMain::windowResized, this, &FeedMessageViewer::onMessageSplitterResized);
}

void FeedMessageViewer::displayMessage(const Message& message, RootItem* root) {
  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::EnableMessagePreview)).toBool()) {
    m_messagesBrowser->loadMessage(message, root);
  }
  else if (m_articleViewerAlwaysVisible) {
    m_messagesBrowser->showItemDetails(root);
  }
  else {
    m_messagesBrowser->clear();
  }
}

void FeedMessageViewer::loadMessageToFeedAndArticleList(Feed* feed, const Message& message) {
  auto idx_src = m_feedsView->sourceModel()->indexForItem(feed);
  auto idx_map = m_feedsView->model()->mapFromSource(idx_src);
  auto is_visible = !m_feedsView->isIndexHidden(idx_map);

  if (!idx_map.isValid() || !is_visible) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("Filtered feed list"),
                                    tr("Cannot select article in article list as your feed is filtered out from feed "
                                       "list."),
                                    QSystemTrayIcon::MessageIcon::Warning),
                         GuiMessageDestination(true, true));
    return;
  }

  // TODO: expand properly
  m_feedsView->setExpanded(idx_map, true);
  m_feedsView->setCurrentIndex(idx_map);
  QCoreApplication::processEvents();

  auto idx_map_msg = m_messagesView->model()->indexFromMessage(message);
  auto msg_is_visible = !m_messagesView->isRowHidden(idx_map_msg.row(), idx_map_msg);

  if (!idx_map_msg.isValid() || !msg_is_visible) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("Filtered article list"),
                                    tr("Cannot select article as it seems your article list is filtered or the article "
                                       "was deleted."),
                                    QSystemTrayIcon::MessageIcon::Warning),
                         GuiMessageDestination(true, true));
    return;
  }

  m_messagesView->setCurrentIndex(idx_map_msg);
}

void FeedMessageViewer::onMessageRemoved(RootItem* root) {
  if (m_articleViewerAlwaysVisible) {
    m_messagesBrowser->showItemDetails(root);
  }
  else {
    m_messagesBrowser->clear();
  }
}

void FeedMessageViewer::createConnections() {
  // Filtering & searching.
  connect(m_toolBarMessages, &MessagesToolBar::searchCriteriaChanged, m_messagesView, &MessagesView::searchMessages);
  connect(m_toolBarFeeds, &FeedsToolBar::searchCriteriaChanged, m_feedsView, &FeedsView::filterItems);
  connect(m_toolBarMessages,
          &MessagesToolBar::messageHighlighterChanged,
          m_messagesView,
          &MessagesView::highlightMessages);

  connect(m_toolBarMessages, &MessagesToolBar::messageFilterChanged, this, &FeedMessageViewer::changeMessageFilter);
  connect(m_toolBarFeeds, &FeedsToolBar::feedFilterChanged, this, &FeedMessageViewer::changeFeedFilter);

  connect(m_feedSplitter, &QSplitter::splitterMoved, this, &FeedMessageViewer::onFeedSplitterResized);
  connect(m_messageSplitter, &QSplitter::splitterMoved, this, &FeedMessageViewer::onMessageSplitterResized);

  connect(m_messagesBrowser,
          &MessagePreviewer::markMessageRead,
          m_messagesView->sourceModel(),
          &MessagesModel::setMessageReadById);
  connect(m_messagesBrowser,
          &MessagePreviewer::markMessageImportant,
          m_messagesView->sourceModel(),
          &MessagesModel::setMessageImportantById);
  connect(m_messagesBrowser,
          &MessagePreviewer::setMessageLabelIds,
          m_messagesView->sourceModel(),
          &MessagesModel::setMessageLabelsById);

  connect(m_messagesView, &MessagesView::currentMessageRemoved, this, &FeedMessageViewer::onMessageRemoved);
  connect(m_messagesView, &MessagesView::currentMessageChanged, this, &FeedMessageViewer::displayMessage);

  // If user selects feeds, load their messages.
  connect(m_feedsView, &FeedsView::itemSelected, m_messagesView, &MessagesView::loadItem);
  connect(m_feedsView, &FeedsView::requestViewNextUnreadMessage, m_messagesView, &MessagesView::selectNextUnreadItem);

  // State of many messages is changed, then we need
  // to reload selections.
  connect(m_feedsView->sourceModel(),
          &FeedsModel::dataChangeNotificationTriggered,
          m_messagesView,
          &MessagesView::reactOnExternalDataChange);
}

void FeedMessageViewer::updateArticleViewerSettings() {
  m_articleViewerAlwaysVisible =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::AlwaysDisplayItemPreview)).toBool();
}

MessagePreviewer* FeedMessageViewer::messagesBrowser() const {
  return m_messagesBrowser;
}

void FeedMessageViewer::initialize() {
  // Initialize/populate toolbars.
  m_toolBarFeeds->setFloatable(false);
  m_toolBarFeeds->setMovable(false);
  m_toolBarFeeds->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);
  m_toolBarMessages->setFloatable(false);
  m_toolBarMessages->setMovable(false);
  m_toolBarMessages->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);

  updateArticleViewerSettings();

  // if (!m_articleViewerAlwaysVisible) {
  m_messagesBrowser->clear();
  //}

  // Now refresh visual setup.
  refreshVisualProperties();
}

void FeedMessageViewer::initializeViews() {
  m_feedsWidget = new QWidget(this);
  m_messagesWidget = new QWidget(this);
  m_feedSplitter = new QSplitter(Qt::Orientation::Horizontal, this);
  m_messageSplitter = new QSplitter(Qt::Orientation::Vertical, this);

  // Instantiate needed components.
  auto* central_layout = new QVBoxLayout(this);
  auto* feed_layout = new QVBoxLayout(m_feedsWidget);
  auto* message_layout = new QVBoxLayout(m_messagesWidget);

  // Set layout properties.
  central_layout->setContentsMargins({});
  feed_layout->setContentsMargins({});
  message_layout->setContentsMargins({});

  central_layout->setSpacing(0);
  feed_layout->setSpacing(0);
  message_layout->setSpacing(0);

  // Set views.
  m_feedsView->setFrameStyle(QFrame::Shape::NoFrame);
  m_messagesView->setFrameStyle(QFrame::Shape::NoFrame);

  // Setup message splitter.
  m_messageSplitter->setObjectName(QSL("m_messageSplitter"));
  m_messageSplitter->setHandleWidth(1);
  m_messageSplitter->setOpaqueResize(true);
  m_messageSplitter->setChildrenCollapsible(false);
  m_messageSplitter->addWidget(m_messagesView);
  m_messageSplitter->addWidget(m_messagesBrowser);

  // Assemble message-related components to single widget.
  message_layout->addWidget(m_toolBarMessages);
  message_layout->addWidget(m_messageSplitter);

  // Assemble feed-related components to another widget.
  feed_layout->addWidget(m_toolBarFeeds);
  feed_layout->addWidget(m_feedsView);

  // Assemble everything together.
  m_feedSplitter->setHandleWidth(1);
  m_feedSplitter->setOpaqueResize(true);
  m_feedSplitter->setChildrenCollapsible(false);
  m_feedSplitter->addWidget(m_feedsWidget);
  m_feedSplitter->addWidget(m_messagesWidget);

  // Add toolbar and main feeds/messages widget to main layout.
  central_layout->addWidget(m_feedSplitter);
  setTabOrder(m_feedsView, m_messagesView);
  setTabOrder(m_messagesView, m_toolBarFeeds);
  setTabOrder(m_toolBarFeeds, m_toolBarMessages);
  setTabOrder(m_toolBarMessages, m_messagesBrowser);

  // Set initial ratio of sizes.
  m_feedSplitter->setStretchFactor(0, 1);
  m_feedSplitter->setStretchFactor(1, 3);
}

void FeedMessageViewer::refreshVisualProperties() {
  const Qt::ToolButtonStyle button_style =
    static_cast<Qt::ToolButtonStyle>(qApp->settings()->value(GROUP(GUI), SETTING(GUI::ToolbarStyle)).toInt());

  m_toolBarFeeds->setToolButtonStyle(button_style);
  m_toolBarMessages->setToolButtonStyle(button_style);

  const int icon_size = qApp->settings()->value(GROUP(GUI), SETTING(GUI::ToolbarIconSize)).toInt();

  if (icon_size > 0) {
    m_toolBarFeeds->setIconSize({icon_size, icon_size});
  }
  else {
    m_toolBarFeeds->setIconSize({qApp->style()->pixelMetric(QStyle::PM_ToolBarIconSize),
                                 qApp->style()->pixelMetric(QStyle::PM_ToolBarIconSize)});
  }

  m_toolBarMessages->setIconSize(m_toolBarFeeds->iconSize());
}
