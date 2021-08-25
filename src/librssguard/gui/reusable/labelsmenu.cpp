// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/labelsmenu.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCheckBox>
#include <QKeyEvent>
#include <QPainter>

LabelsMenu::LabelsMenu(const QList<Message>& messages, const QList<Label*>& labels, QWidget* parent)
  : NonClosableMenu(tr("Labels"), parent), m_messages(messages) {
  setIcon(qApp->icons()->fromTheme(QSL("tag-folder")));

  if (labels.isEmpty()) {
    QAction* act_not_labels = new QAction(tr("No labels found"));

    act_not_labels->setEnabled(false);
    addAction(act_not_labels);
  }
  else {
    QSqlDatabase db = qApp->database()->driver()->connection(QSL("LabelsMenu"));

    for (Label* label: boolinq::from(labels).orderBy([](const Label* label) {
      return label->title().toLower();
    }).toStdList()) {

      auto count = boolinq::from(messages).count([&db, label](const Message& msg) {
        return DatabaseQueries::isLabelAssignedToMessage(db, label, msg);
      });

      Qt::CheckState state = Qt::CheckState::Unchecked;

      if (count == messages.size()) {
        state = Qt::CheckState::Checked;
      }
      else if (count > 0) {
        state = Qt::CheckState::PartiallyChecked;
      }

      addLabelAction(label, state);
    }
  }
}

void LabelsMenu::keyPressEvent(QKeyEvent* event) {
  LabelAction* act = qobject_cast<LabelAction*>(activeAction());

  if (act != nullptr && event->key() == Qt::Key::Key_Space) {
    act->toggleCheckState();
  }

  NonClosableMenu::keyPressEvent(event);
}

void LabelsMenu::mousePressEvent(QMouseEvent* event) {
  LabelAction* act = qobject_cast<LabelAction*>(activeAction());

  if (act != nullptr) {
    act->toggleCheckState();
  }
  else {
    NonClosableMenu::mousePressEvent(event);
  }
}

void LabelsMenu::changeLabelAssignment(Qt::CheckState state) {
  LabelAction* origin = qobject_cast<LabelAction*>(sender());

  if (origin != nullptr) {
    if (state == Qt::CheckState::Checked) {
      // Assign this label to selected messages.
      for (const auto& msg : qAsConst(m_messages)) {
        origin->label()->assignToMessage(msg);
      }
    }
    else if (state == Qt::CheckState::Unchecked) {
      // Remove label from selected messages.
      for (const auto& msg : qAsConst(m_messages)) {
        origin->label()->deassignFromMessage(msg);
      }
    }
  }

  emit labelsChanged();
}

void LabelsMenu::addLabelAction(Label* label, Qt::CheckState state) {
  auto* act = new LabelAction(label, this, this);

  act->setCheckState(state);
  addAction(act);

  connect(act, &LabelAction::checkStateChanged, this, &LabelsMenu::changeLabelAssignment);
}

LabelAction::LabelAction(Label* label, QWidget* parent_widget, QObject* parent)
  : QAction(parent), m_label(label), m_parentWidget(parent_widget), m_checkState(Qt::CheckState::Unchecked) {
  setText(label->title());
  setIconVisibleInMenu(true);
  setIcon(label->icon());

  connect(this, &LabelAction::checkStateChanged, this, &LabelAction::updateActionForState);
  updateActionForState();
}

Qt::CheckState LabelAction::checkState() const {
  return m_checkState;
}

void LabelAction::setCheckState(Qt::CheckState state) {
  if (state != m_checkState) {
    m_checkState = state;
    emit checkStateChanged(m_checkState);
  }
}

Label* LabelAction::label() const {
  return m_label;
}

void LabelAction::toggleCheckState() {
  if (m_checkState == Qt::CheckState::Unchecked) {
    setCheckState(Qt::CheckState::Checked);
  }
  else {
    setCheckState(Qt::CheckState::Unchecked);
  }
}

void LabelAction::updateActionForState() {
  QColor highlight;

  switch (m_checkState) {
    case Qt::CheckState::Checked:
      highlight = Qt::GlobalColor::green;
      break;

    case Qt::CheckState::PartiallyChecked:
      highlight = QColor(100, 50, 0);
      break;

    case Qt::CheckState::Unchecked:
    default:
      highlight = Qt::GlobalColor::transparent;
      break;
  }

  QPixmap copy_icon(m_label->icon().pixmap(48, 48));

  if (m_checkState != Qt::CheckState::Unchecked) {
    QPainter paint(&copy_icon);

    paint.setPen(QPen(Qt::GlobalColor::black, 4.0f));
    paint.setBrush(highlight);
    paint.drawRect(0, 0, 22, 22);
  }

  setIcon(copy_icon);
}
