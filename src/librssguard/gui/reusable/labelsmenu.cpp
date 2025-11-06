// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/labelsmenu.h"

#include "3rd-party/boolinq/boolinq.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCheckBox>
#include <QKeyEvent>
#include <QPainter>

LabelsMenu::LabelsMenu(QWidget* parent) : ScrollableMenu(tr("Labels"), parent) {
  setIcon(qApp->icons()->fromTheme(QSL("tag-new"), QSL("tag-edit")));
}

void LabelsMenu::setLabels(const QList<Label*>& labels) {
  m_labelActions.clear();

  if (labels.isEmpty()) {
    QAction* act_not_labels = new QAction(tr("No labels found"));

    act_not_labels->setEnabled(false);
    setActions({act_not_labels}, false);
  }
  else {
    auto lbls =
      boolinq::from(labels)
        .select([this](Label* label) {
          return labelAction(label);
        })
        .orderBy([](const QAction* label_action) {
          return QSL("%1%2").arg(label_action->isChecked() ? QSL("0") : QSL("1"), label_action->text().toLower());
        })
        .toStdList();

    m_labelActions = FROM_STD_LIST(QList<QAction*>, lbls);
    setActions(m_labelActions, false);
  }
}

void LabelsMenu::changeLabelAssignment(bool assign) {
  LabelAction* origin = qobject_cast<LabelAction*>(sender());
  auto lbl = origin->label();

  if (origin != nullptr && lbl != nullptr) {
    try {
      QList<int> article_ids;
      QList<QList<Label*>> article_labels;

      article_ids.reserve(m_messages.size());
      article_labels.reserve(m_messages.size());

      for (auto& msg : m_messages) {
        // NOTE: To avoid duplicates.
        msg.m_assignedLabels.removeAll(lbl);

        if (assign) {
          lbl->assignToMessage(msg, false);
          msg.m_assignedLabels.append(lbl);
        }
        else {
          lbl->deassignFromMessage(msg, false);
          msg.m_assignedLabels.removeOne(lbl);
        }

        article_ids.append(msg.m_id);
        article_labels.append(msg.m_assignedLabels);
      }

      lbl->account()->onAfterLabelMessageAssignmentChanged({lbl}, m_messages, assign);
      emit setModelArticleLabelIds(article_ids, article_labels);
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
  auto* act = new LabelAction(label, this);

  act->setCheckable(true);
  act->setChecked(act->isCheckable() && boolinq::from(m_messages).all([&](const Message& msg) {
    return msg.m_assignedLabels.contains(label);
  }));

  connect(act, &LabelAction::toggled, this, &LabelsMenu::changeLabelAssignment);

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
