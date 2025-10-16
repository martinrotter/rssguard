// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/labelsmenu.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCheckBox>
#include <QKeyEvent>
#include <QPainter>

LabelsMenu::LabelsMenu(Operation operation, QWidget* parent)
  : ScrollableMenu(operation == Operation::AddLabel ? tr("Add labels") : tr("Remove labels"), parent),
    m_operation(operation) {
  setIcon((m_operation == Operation::AddLabel || m_operation == Operation::Toggle)
            ? qApp->icons()->fromTheme(QSL("tag-new"), QSL("tag-edit"))
            : qApp->icons()->fromTheme(QSL("tag-delete"), QSL("tag-reset")));
}

void LabelsMenu::setLabels(const QList<Label*>& labels) {
  m_labelActions.clear();

  if (labels.isEmpty()) {
    QAction* act_not_labels = new QAction(tr("No labels found"));

    act_not_labels->setEnabled(false);
    setActions({act_not_labels}, false);
  }
  else {
    auto lbls = boolinq::from(labels)
                  .orderBy([](const Label* label) {
                    return label->title().toLower();
                  })
                  .select([this](Label* label) {
                    return labelAction(label);
                  })
                  .toStdList();

    m_labelActions = FROM_STD_LIST(QList<QAction*>, lbls);
    setActions(m_labelActions, false);
  }
}

void LabelsMenu::changeLabelAssignment(bool assign) {
  LabelAction* origin = qobject_cast<LabelAction*>(sender());
  auto lbl = origin->label();

  if (origin != nullptr) {
    if (m_operation != Operation::Toggle) {
      origin->setEnabled(false);
    }

    for (auto& msg : m_messages) {
      if (m_operation == Operation::AddLabel || (m_operation == Operation::Toggle && assign)) {
        origin->label()->assignToMessage(msg, true);
        msg.m_assignedLabelsIds.append(lbl->customId());
      }
      else {
        origin->label()->deassignFromMessage(msg, true);
        msg.m_assignedLabelsIds.removeOne(lbl->customId());
      }

      emit setModelArticleLabelIds(msg.m_id, msg.m_assignedLabelsIds);
    }
  }
}

QAction* LabelsMenu::labelAction(Label* label) {
  auto* act = new LabelAction(label, this);

  act->setCheckable(m_operation == Operation::Toggle);
  act->setChecked(act->isCheckable() && boolinq::from(m_messages).all([&](const Message& msg) {
    return msg.m_assignedLabelsIds.contains(label->customId());
  }));

  connect(act, &LabelAction::triggered, this, &LabelsMenu::changeLabelAssignment);

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
