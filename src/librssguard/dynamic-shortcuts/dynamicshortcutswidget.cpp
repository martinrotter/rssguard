// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/dynamicshortcutswidget.h"

#include "definitions/definitions.h"
#include "dynamic-shortcuts/shortcutcatcher.h"
#include "dynamic-shortcuts/shortcutpriority.h"
#include "gui/messagebox.h"

#include <QAction>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>

DynamicShortcutsWidget::DynamicShortcutsWidget(QWidget* parent) : QWidget(parent) {
  // Create layout for this control and set is as active.
  m_layout = new QGridLayout(this);
  m_layout->setContentsMargins({});

  m_conflictWarning = new QLabel(this);
  m_conflictWarning->setTextFormat(Qt::PlainText);
  m_conflictWarning->setWordWrap(true);
  m_conflictWarning->hide();
  m_layout->addWidget(m_conflictWarning, 0, 0, 1, 3);

  setLayout(m_layout);
}

DynamicShortcutsWidget::~DynamicShortcutsWidget() {
  delete m_layout;
}

void DynamicShortcutsWidget::updateShortcuts() {
  const QPointer<DynamicShortcutsWidget> self(this);
  QList<QPointer<ShortcutCatcher>> bindings;

  for (ShortcutCatcher* catcher : std::as_const(m_actionBindings)) {
    bindings.append(catcher);
  }

  // Apply can be clicked before Qt's one-second recording timer has finished.
  // Resolve those edits before mutating any live action.
  for (const auto& catcher : std::as_const(bindings)) {
    if (!catcher.isNull()) {
      catcher->finishRecording();
    }

    if (self.isNull()) {
      return;
    }
  }

  for (const auto& catcher : std::as_const(bindings)) {
    if (!catcher.isNull() && catcher->action() != nullptr) {
      catcher->action()->setShortcut(catcher->shortcut());
    }

    if (self.isNull()) {
      return;
    }
  }
}

void DynamicShortcutsWidget::populate(QList<QAction*> actions) {
  m_actionBindings.clear();
  m_stagedShortcuts.clear();

  // Settings can reload this page after Apply. Remove the previous recorders.
  while (m_layout->count() > 1) {
    QLayoutItem* item = m_layout->takeAt(1);
    delete item->widget();
    delete item;
  }

  for (int row = 1; row < m_layout->rowCount(); ++row) {
    m_layout->setRowStretch(row, 0);
  }

  actions.erase(std::remove(actions.begin(), actions.end(), nullptr), actions.end());
  std::sort(actions.begin(), actions.end(), [](QAction* lhs, QAction* rhs) {
    return QString::localeAwareCompare(lhs->text().replace(QL1S("&"), QString()),
                                       rhs->text().replace(QL1S("&"), QString())) < 0;
  });
  int row_id = 1;

  for (QAction* action : actions) {
    // Create shortcut catcher for this action and set default shortcut.
    auto* catcher = new ShortcutCatcher(this);

    catcher->setAction(action);
    catcher->setDefaultShortcut(action->shortcut());

    m_stagedShortcuts.insert(catcher, action->shortcut());

    m_actionBindings.append(catcher);

    // Add new catcher to our control.
    auto* action_label = new QLabel(this);
    auto act_text = action->text().remove(QSL("&"));
    auto act_toolt = action->toolTip();

    action_label->setText(act_text.isEmpty() ? act_toolt : act_text);
    action_label->setToolTip(action->toolTip());
    action_label->setWordWrap(true);

    auto* action_icon = new QLabel(this);

    action_icon->setPixmap(action->icon().pixmap(ICON_SIZE_SETTINGS, ICON_SIZE_SETTINGS));
    action_icon->setToolTip(action->toolTip());

    m_layout->addWidget(action_icon, row_id, 0);
    m_layout->addWidget(action_label, row_id, 1);
    m_layout->addWidget(catcher, row_id, 2);

    row_id++;
    connect(catcher, &ShortcutCatcher::shortcutChanged, this, &DynamicShortcutsWidget::onShortcutChanged);
  }

  // Make sure that "spacer" is added.
  m_layout->setRowStretch(row_id, 1);
  m_layout->setColumnStretch(1, 1);
  updateConflictWarning();
}

void DynamicShortcutsWidget::onShortcutChanged(const QKeySequence& sequence) {
  auto* catcher = qobject_cast<ShortcutCatcher*>(sender());

  if (catcher == nullptr || !m_stagedShortcuts.contains(catcher)) {
    return;
  }

  const QKeySequence previous = m_stagedShortcuts.value(catcher);
  const QPointer<DynamicShortcutsWidget> self(this);
  const QPointer<ShortcutCatcher> edited(catcher);
  QList<QPointer<ShortcutCatcher>> conflicts;
  QStringList details;

  for (ShortcutCatcher* other : std::as_const(m_actionBindings)) {
    if (other != catcher && other->action() != nullptr &&
        ShortcutPriority::sequencesConflict(sequence, m_stagedShortcuts.value(other))) {
      conflicts.append(other);
      details.append(QSL("%1: %2").arg(other->action()->text().remove(QSL("&")),
                                       m_stagedShortcuts.value(other).toString(QKeySequence::NativeText)));
    }
  }

  if (!conflicts.isEmpty()) {
    const auto decision = MsgBox::show(this,
                                       QMessageBox::Icon::Warning,
                                       tr("Conflicting shortcuts"),
                                       tr("The shortcut %1 conflicts with other assignments.")
                                         .arg(sequence.toString(QKeySequence::NativeText)),
                                       tr("Keep the new assignment and clear all conflicting shortcuts?"),
                                       details.join(QL1C('\n')),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::Yes);

    if (self.isNull() || edited.isNull()) {
      return;
    }

    if (decision != QMessageBox::Yes) {
      const QSignalBlocker blocker(catcher);
      catcher->setShortcut(previous);
      return;
    }

    for (const auto& other : std::as_const(conflicts)) {
      if (!other.isNull()) {
        const QSignalBlocker blocker(other.data());
        other->clearShortcut();
        m_stagedShortcuts.insert(other.data(), {});
      }
    }
  }

  m_stagedShortcuts.insert(catcher, sequence);
  updateConflictWarning();
  emit setupChanged();
}

void DynamicShortcutsWidget::updateConflictWarning() {
  QStringList conflicts;

  for (int i = 0; i < m_actionBindings.size(); ++i) {
    const auto* first = m_actionBindings.at(i);

    for (int j = i + 1; j < m_actionBindings.size(); ++j) {
      const auto* second = m_actionBindings.at(j);

      if (first->action() != nullptr && second->action() != nullptr &&
          ShortcutPriority::sequencesConflict(first->shortcut(), second->shortcut())) {
        conflicts.append(tr("%1 (%2) conflicts with %3 (%4)")
                           .arg(first->action()->text().remove(QSL("&")),
                                first->shortcut().toString(QKeySequence::NativeText),
                                second->action()->text().remove(QSL("&")),
                                second->shortcut().toString(QKeySequence::NativeText)));
      }
    }
  }

  m_conflictWarning
    ->setText(tr("Conflicting assignments need to be changed or cleared:\n%1").arg(conflicts.join(QL1C('\n'))));
  m_conflictWarning->setVisible(!conflicts.isEmpty());

  if (!conflicts.isEmpty()) {
    qWarningNN << "Conflicting configured shortcuts:" << conflicts;
  }
}
