// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/dynamicshortcutswidget.h"

#include "definitions/definitions.h"
#include "dynamic-shortcuts/shortcutcatcher.h"
#include "gui/messagebox.h"

#include <QAction>
#include <QGridLayout>
#include <QLabel>

DynamicShortcutsWidget::DynamicShortcutsWidget(QWidget* parent) : QWidget(parent) {
  // Create layout for this control and set is as active.
  m_layout = new QGridLayout(this);
  m_layout->setContentsMargins({});

  setLayout(m_layout);
}

DynamicShortcutsWidget::~DynamicShortcutsWidget() {
  delete m_layout;
}

bool DynamicShortcutsWidget::areShortcutsUnique() const {
  QList<QKeySequence> all_shortcuts;

  // Obtain all shortcuts.
  for (ShortcutCatcher* catcher : m_actionBindings) {
    const QKeySequence new_shortcut = catcher->shortcut();

    if (!new_shortcut.isEmpty() && all_shortcuts.contains(new_shortcut)) {
      // Problem, two identical non-empty shortcuts found.
      return false;
    }
    else {
      all_shortcuts.append(catcher->shortcut());
    }
  }

  return true;
}

void DynamicShortcutsWidget::updateShortcuts() {
  for (ShortcutCatcher* catcher : std::as_const(m_actionBindings)) {
    catcher->action()->setShortcut(catcher->shortcut());
  }
}

void DynamicShortcutsWidget::populate(QList<QAction*> actions) {
  m_actionBindings.clear();
  std::sort(actions.begin(), actions.end(), [](QAction* lhs, QAction* rhs) {
    return QString::localeAwareCompare(lhs->text().replace(QL1S("&"), QString()),
                                       rhs->text().replace(QL1S("&"), QString())) < 0;
  });
  int row_id = 0;

  for (QAction* action : actions) {
    // Create shortcut catcher for this action and set default shortcut.
    auto* catcher = new ShortcutCatcher(this);

    catcher->setAction(action);
    catcher->setDefaultShortcut(action->shortcut());

    if (!action->shortcut().isEmpty()) {
      m_assignedShortcuts.insert(action->shortcut(), catcher);
    }

    m_actionBindings.append(catcher);

    // Add new catcher to our control.
    auto* action_label = new QLabel(this);
    auto act_text = action->text().remove(QSL("&"));
    auto act_toolt = action->toolTip();

    if (act_toolt.isEmpty() || act_text == act_toolt) {
      action_label->setText(act_text);
    }
    else {
      action_label->setText(act_toolt);
      // action_label->setText(QSL("%1 (%2)").arg(act_text, act_toolt));
    }

    action_label->setToolTip(action->toolTip());
    action_label->setWordWrap(true);

    // action_label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

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
}

void DynamicShortcutsWidget::onShortcutChanged(const QKeySequence& sequence) {
  ShortcutCatcher* catcher = qobject_cast<ShortcutCatcher*>(sender());
  QKeySequence assigned_sequence = m_assignedShortcuts.key(catcher);

  qDebugNN << catcher->action()->text();

  // We remove currently assigned sequence to this catcher.
  m_assignedShortcuts.remove(assigned_sequence);

  if (!sequence.isEmpty()) {
    // Now we check if any other catcher has assigned the same sequence.
    ShortcutCatcher* conflicting_catcher = m_assignedShortcuts.value(sequence);

    if (conflicting_catcher != nullptr) {
      // We found conflicting catcher.
      catcher->blockSignals(true);

      QMessageBox::StandardButton decision =
        MsgBox::show(this,
                     QMessageBox::Icon::Critical,
                     tr("Duplicate shortcut"),
                     tr("There is another action which has the same shortcut assigned."),
                     tr("Do you want to keep the new shortcut assignment and clear the previous shortcut?"),
                     conflicting_catcher->action()->text().remove(QSL("&")),
                     QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                     QMessageBox::StandardButton::Yes);

      catcher->blockSignals(false);

      if (decision == QMessageBox::StandardButton::Yes) {
        // We keep the new shortcut and reset old conflicting action.
        m_assignedShortcuts.insert(sequence, catcher);
        conflicting_catcher->clearShortcut();
      }
      else {
        // We discard new shortcut.
        catcher->clearShortcut();
      }
    }
    else {
      m_assignedShortcuts.insert(sequence, catcher);
    }
  }

  emit setupChanged();
}
