// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/labelsmenu.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCheckBox>
#include <QKeyEvent>
#include <QPainter>

LabelsMenu::LabelsMenu(Operation operation,
                       const QList<Message>& messages,
                       const QList<Label*>& labels,
                       QWidget* parent)
  : ScrollableMenu(operation == Operation::AddLabel ? tr("Add labels") : tr("Remove labels"), parent),
    m_messages(messages), m_operation(operation) {
  setIcon(m_operation == Operation::AddLabel ? qApp->icons()->fromTheme(QSL("tag-new"), QSL("tag-edit"))
                                             : qApp->icons()->fromTheme(QSL("tag-delete"), QSL("tag-reset")));

  if (labels.isEmpty()) {
    QAction* act_not_labels = new QAction(tr("No labels found"));

    act_not_labels->setEnabled(false);
    setActions({act_not_labels}, false);
  }
  else {
    QList<QAction*> lbls;

    for (Label* label : boolinq::from(labels)
                          .orderBy([](const Label* label) {
                            return label->title().toLower();
                          })
                          .toStdList()) {
      lbls.append(labelAction(label));
    }

    setActions(lbls, false);
  }
}

void LabelsMenu::changeLabelAssignment() {
  LabelAction* origin = qobject_cast<LabelAction*>(sender());

  origin->setEnabled(false);

  if (origin != nullptr) {
    if (m_operation == Operation::AddLabel) {
      // Assign this label to selected messages.
      for (const auto& msg : std::as_const(m_messages)) {
        origin->label()->assignToMessage(msg, true);
      }
    }
    else {
      // Remove label from selected messages.
      for (const auto& msg : std::as_const(m_messages)) {
        origin->label()->deassignFromMessage(msg, true);
      }
    }
  }

  emit labelsChanged();
}

QAction* LabelsMenu::labelAction(Label* label) {
  auto* act = new LabelAction(label, this, this);

  act->setCheckable(true);
  connect(act, &LabelAction::triggered, this, &LabelsMenu::changeLabelAssignment);

  return act;
}

LabelAction::LabelAction(Label* label, QWidget* parent_widget, QObject* parent) : QAction(parent), m_label(label) {
  setText(label->title());
  setIconVisibleInMenu(true);
  setIcon(label->icon());
}

Label* LabelAction::label() const {
  return m_label;
}
