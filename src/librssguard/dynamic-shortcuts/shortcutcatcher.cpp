// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/shortcutcatcher.h"

#include "dynamic-shortcuts/shortcutsequenceedit.h"
#include "gui/reusable/plaintoolbutton.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QSignalBlocker>

ShortcutCatcher::ShortcutCatcher(QWidget* parent) : QWidget(parent) {
  // Setup layout of the control
  m_layout = new QHBoxLayout(this);

  m_layout->setContentsMargins({});
  m_layout->setSpacing(1);

  // Create reset button.
  m_btnReset = new PlainToolButton(this);
  m_btnReset->setIcon(qApp->icons()->fromTheme(QSL("document-revert")));
  m_btnReset->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_btnReset->setToolTip(tr("Undo shortcut"));

  // Create clear button.
  m_btnClear = new PlainToolButton(this);
  m_btnClear->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));
  m_btnClear->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_btnClear->setToolTip(tr("Clear current shortcut"));

  // Clear main shortcut catching button.
  m_shortcutBox = new ShortcutSequenceEdit(this);
  m_shortcutBox->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_shortcutBox->setMinimumWidth(170);
  m_shortcutBox->setToolTip(tr("Click and hit new shortcut."));

  // Add all buttons to the layout.
  m_layout->addWidget(m_shortcutBox);
  m_layout->addWidget(m_btnReset);
  m_layout->addWidget(m_btnClear);

  // Establish needed connections.
  connect(m_btnReset, &QToolButton::clicked, this, &ShortcutCatcher::resetShortcut);
  connect(m_btnClear, &QToolButton::clicked, this, &ShortcutCatcher::clearShortcut);
  // Validate the completed sequence, not provisional prefixes or the empty
  // sequence Qt emits when a new recording begins.
  connect(m_shortcutBox, &QKeySequenceEdit::editingFinished, this, &ShortcutCatcher::finishRecording);
}

QKeySequence ShortcutCatcher::shortcut() const {
  return m_currentSequence;
}

void ShortcutCatcher::setDefaultShortcut(const QKeySequence& key) {
  m_defaultSequence = key;
  setShortcut(key);
}

void ShortcutCatcher::setShortcut(const QKeySequence& key) {
  {
    const QSignalBlocker blocker(m_shortcutBox);
    m_shortcutBox->setKeySequence(key);
  }

  if (m_currentSequence != key) {
    m_currentSequence = key;
    emit shortcutChanged(key);
  }
}

void ShortcutCatcher::finishRecording() {
  setShortcut(m_shortcutBox->keySequence());
}

void ShortcutCatcher::resetShortcut() {
  setShortcut(m_defaultSequence);
}

void ShortcutCatcher::clearShortcut() {
  setShortcut(QKeySequence());
}

QAction* ShortcutCatcher::action() const {
  return m_action;
}

void ShortcutCatcher::setAction(QAction* act) {
  m_action = act;
}
