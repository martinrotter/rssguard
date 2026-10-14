// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/labelsmenu.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "qtlinq/qtlinq.h"

#include <QCheckBox>
#include <QKeyEvent>
#include <QPainter>

LabelsMenu::LabelsMenu(QWidget* parent) : ScrollableMenu(tr("Labels"), parent) {
  setIcon(qApp->icons()->fromTheme(QSL("tag-new"), QSL("tag-edit")));
}

void LabelsMenu::setLabels(const QList<Label*>& labels) {
  for (auto* act : m_labelActions) {
    act->setEnabled(false);
    QObject::disconnect(act, nullptr, act, nullptr);
  }

  if (m_actionsOwner != nullptr) {
    m_actionsOwner->deleteLater();
  }

  // QMenu::clear() must only detach these actions, not own their lifetime.
  m_actionsOwner = new QObject(this);
  m_labelActions.clear();

  if (labels.isEmpty()) {
    QAction* act_not_labels = new QAction(qApp->icons()->fromTheme(QSL("tag")), tr("No labels found"), m_actionsOwner);

    act_not_labels->setEnabled(false);
    setActions({act_not_labels}, false);
  }
  else {
    m_labelActions =
      qlinq::from(labels)
        .select([this](Label* label) {
          return labelAction(label);
        })
        .orderBy([](const QAction* label_action) {
          return QSL("%1%2").arg(label_action->isChecked() ? QSL("0") : QSL("1"), label_action->text().toLower());
        })
        .toList();

    setActions(m_labelActions, false);
  }
}

void LabelsMenu::changeLabelAssignment(LabelAction* origin, bool assign) {
  auto* lbl = origin != nullptr ? origin->label() : nullptr;

  if (origin != nullptr && lbl != nullptr) {
    try {
      if (assign) {
        lbl->assignToMessages(m_messages, false);
      }
      else {
        lbl->deassignFromMessages(m_messages, false);
      }

      for (auto& msg : m_messages) {
        msg.m_assignedLabels.removeAll(lbl);

        if (assign) {
          msg.m_assignedLabels.append(lbl);
        }
      }

      lbl->account()->onAfterLabelMessageAssignmentChanged({lbl}, m_messages, assign);
      emit setModelArticleLabelIds(m_messages);
    }
    catch (const ApplicationException& ex) {
      qCriticalNN << LOGSEC_CORE << "Failed to (de)assign label to/from article:" << NONQUOTE_W_SPACE_DOT(ex.message());
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           GuiMessage(tr("Cannot change labels"),
                                      tr("Failed to (de)assign label to/from article: %1.").arg(ex.message()),
                                      QSystemTrayIcon::MessageIcon::Critical),
                           GuiMessageDestination(true, true));
    }
  }
}

QAction* LabelsMenu::labelAction(Label* label) {
  auto* act = new LabelAction(label, m_actionsOwner);

  act->setCheckable(true);
  act->setChecked(act->isCheckable() && qlinq::from(m_messages).all([&](const Message& msg) {
    return msg.m_assignedLabels.contains(label);
  }));

  // QMenu::removeAction() disconnects signals to the menu itself.
  connect(act, &QAction::toggled, act, [this, act](bool assign) {
    changeLabelAssignment(act, assign);
  });

  return act;
}

QList<QAction*> LabelsMenu::labelActions() const {
  return m_labelActions;
}

QList<Message> LabelsMenu::messages() const {
  return m_messages;
}

void LabelsMenu::setMessages(const QList<Message>& messages) {
  m_messages = messages;
}

LabelAction::LabelAction(Label* label, QObject* parent) : QAction(parent), m_label(label) {
  setText(label->title());
  setIconVisibleInMenu(true);
  setIcon(label->icon());
  setToolTip(label->title());
}

Label* LabelAction::label() const {
  return m_label;
}
