// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagepreviewer.h"

#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/plaintoolbutton.h"
#include "gui/searchtextwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "network-web/webfactory.h"
#include "services/abstract/label.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/serviceroot.h"

#if defined (USE_WEBENGINE)
#include "gui/webbrowser.h"
#include "gui/webviewer.h"
#else
#include "gui/messagebrowser.h"
#endif

#include <QCheckBox>
#include <QGridLayout>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QToolBar>
#include <QToolTip>

void MessagePreviewer::createConnections() {
  installEventFilter(this);

  connect(m_actionMarkRead = m_toolBar->addAction(qApp->icons()->fromTheme("mail-mark-read"), tr("Mark message as read")),
          &QAction::triggered,
          this,
          &MessagePreviewer::markMessageAsRead);
  connect(m_actionMarkUnread = m_toolBar->addAction(qApp->icons()->fromTheme("mail-mark-unread"), tr("Mark message as unread")),
          &QAction::triggered,
          this,
          &MessagePreviewer::markMessageAsUnread);
  connect(m_actionSwitchImportance = m_toolBar->addAction(qApp->icons()->fromTheme("mail-mark-important"), tr("Switch message importance")),
          &QAction::triggered,
          this,
          &MessagePreviewer::switchMessageImportance);
}

MessagePreviewer::MessagePreviewer(bool should_resize_to_fit, QWidget* parent)
  : QWidget(parent), m_layout(new QGridLayout(this)), m_toolBar(new QToolBar(this)), m_verticalScrollBarPosition(0.0),
  m_separator(nullptr), m_btnLabels(QList<QPair<LabelButton*, QAction*>>()) {
#if defined (USE_WEBENGINE)
  m_txtMessage = new WebBrowser(this);

  if (should_resize_to_fit) {
    m_txtMessage->setFixedHeight(parent->parentWidget()->height());
  }
#else
  m_txtMessage = new MessageBrowser(should_resize_to_fit, this);
#endif

  // NOTE: To make sure that if we have many labels and short message
  m_toolBar->setSizePolicy(m_toolBar->sizePolicy().horizontalPolicy(), QSizePolicy::Policy::MinimumExpanding);

  m_toolBar->setOrientation(Qt::Orientation::Vertical);
  m_layout->setContentsMargins(3, 3, 3, 3);
  m_layout->addWidget(m_txtMessage, 0, 1, 1, 1);
  m_layout->addWidget(m_toolBar, 0, 0, -1, 1);

  createConnections();
  m_actionSwitchImportance->setCheckable(true);

  reloadFontSettings();
  clear();
}

void MessagePreviewer::reloadFontSettings() {
  m_txtMessage->reloadFontSettings();
}

#if defined (USE_WEBENGINE)

WebBrowser* MessagePreviewer::webBrowser() const {
  return m_txtMessage;
}

#endif

void MessagePreviewer::clear() {
  updateLabels(true);
  m_txtMessage->clear();
  hide();

  m_verticalScrollBarPosition = 0.0;
  m_root.clear();
  m_message = Message();
}

void MessagePreviewer::hideToolbar() {
  m_toolBar->setVisible(false);
}

void MessagePreviewer::loadMessage(const Message& message, RootItem* root) {
  m_verticalScrollBarPosition = m_txtMessage->verticalScrollBarPosition();

  bool same_message = message.m_id == m_message.m_id && m_root == root;

  m_message = message;
  m_root = root;

  if (!m_root.isNull()) {
    updateButtons();
    updateLabels(false);
    show();
    m_actionSwitchImportance->setChecked(m_message.m_isImportant);
    m_txtMessage->loadMessage(message, m_root);

    if (same_message) {
      m_txtMessage->setVerticalScrollBarPosition(m_verticalScrollBarPosition);
    }
  }
}

void MessagePreviewer::switchLabel(bool assign) {
  auto lbl = qobject_cast<LabelButton*>(sender())->label();

  if (lbl == nullptr) {
    return;
  }

  if (assign) {
    lbl->assignToMessage(m_message);
  }
  else {
    lbl->deassignFromMessage(m_message);
  }
}

void MessagePreviewer::markMessageAsRead() {
  markMessageAsReadUnread(RootItem::ReadStatus::Read);
}

void MessagePreviewer::markMessageAsUnread() {
  markMessageAsReadUnread(RootItem::ReadStatus::Unread);
}

void MessagePreviewer::markMessageAsReadUnread(RootItem::ReadStatus read) {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSetMessagesRead(m_root.data(),
                                                                QList<Message>() << m_message,
                                                                read)) {
      DatabaseQueries::markMessagesReadUnread(qApp->database()->connection(objectName(), DatabaseFactory::DesiredType::FromSettings),
                                              QStringList() << QString::number(m_message.m_id),
                                              read);
      m_root->getParentServiceRoot()->onAfterSetMessagesRead(m_root.data(),
                                                             QList<Message>() << m_message,
                                                             read);
      m_message.m_isRead = read == RootItem::ReadStatus::Read;
      emit markMessageRead(m_message.m_id, read);

      updateButtons();
    }
  }
}

void MessagePreviewer::switchMessageImportance(bool checked) {
  if (!m_root.isNull()) {
    if (m_root->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_root.data(),
                                                                        QList<ImportanceChange>()
                                                                        << ImportanceChange(m_message,
                                                                                            m_message.
                                                                                            m_isImportant
                                                                                            ? RootItem::Importance::NotImportant
                                                                                            : RootItem::Importance::Important))) {
      DatabaseQueries::switchMessagesImportance(qApp->database()->connection(objectName(), DatabaseFactory::DesiredType::FromSettings),
                                                QStringList() << QString::number(m_message.m_id));
      m_root->getParentServiceRoot()->onAfterSwitchMessageImportance(m_root.data(),
                                                                     QList<ImportanceChange>()
                                                                     << ImportanceChange(m_message,
                                                                                         m_message.m_isImportant
                                                                                         ? RootItem::Importance::NotImportant
                                                                                         : RootItem::Importance::Important));
      emit markMessageImportant(m_message.m_id, checked
                                ? RootItem::Importance::Important
                                : RootItem::Importance::NotImportant);

      m_message.m_isImportant = checked;
    }
  }
}

void MessagePreviewer::updateButtons() {
  m_actionMarkRead->setEnabled(!m_message.m_isRead);
  m_actionMarkUnread->setEnabled(m_message.m_isRead);
}

void MessagePreviewer::updateLabels(bool only_clear) {
  for (auto& lbl : m_btnLabels) {
    m_toolBar->removeAction(lbl.second);
    lbl.second->deleteLater();
    lbl.first->deleteLater();
  }

  m_btnLabels.clear();

  if (m_separator != nullptr) {
    m_toolBar->removeAction(m_separator);
  }

  if (only_clear) {
    return;
  }

  if (m_root.data() != nullptr && m_root.data()->getParentServiceRoot()->labelsNode()->labels().size() > 0) {
    m_separator = m_toolBar->addSeparator();
    QSqlDatabase database = qApp->database()->connection(metaObject()->className());

    for (auto* label : m_root.data()->getParentServiceRoot()->labelsNode()->labels()) {
      LabelButton* btn_label = new LabelButton(this);

      btn_label->setLabel(label);
      btn_label->setCheckable(true);
      btn_label->setIcon(Label::generateIcon(label->color()));
      btn_label->setAutoRaise(false);
      btn_label->setText(QSL(" ") + label->title());
      btn_label->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
      btn_label->setChecked(DatabaseQueries::isLabelAssignedToMessage(database, label, m_message));

      QAction* act_label = m_toolBar->addWidget(btn_label);

      connect(btn_label, &QToolButton::toggled, this, &MessagePreviewer::switchLabel);

      m_btnLabels.append({ btn_label, act_label });
    }
  }
}

LabelButton::LabelButton(QWidget* parent) : QToolButton(parent), m_label(nullptr) {}

Label* LabelButton::label() const {
  return m_label.data();
}

void LabelButton::setLabel(Label* label) {
  m_label = label;
}
