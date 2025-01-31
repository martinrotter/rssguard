// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/shortcutcatcher.h"

#include "gui/reusable/plaintoolbutton.h"
#include "miscellaneous/iconfactory.h"

#include <QHBoxLayout>
#include <QKeySequenceEdit>

ShortcutCatcher::ShortcutCatcher(QWidget* parent)
  : QWidget(parent), m_isRecording(false), m_numKey(0), m_modifierKeys(0U) {
  // Setup layout of the control
  m_layout = new QHBoxLayout(this);

  m_layout->setContentsMargins({});
  m_layout->setSpacing(1);

  // Create reset button.
  m_btnReset = new PlainToolButton(this);
  m_btnReset->setIcon(qApp->icons()->fromTheme(QSL("document-revert")));
  m_btnReset->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_btnReset->setToolTip(tr("Reset to original shortcut."));

  // Create clear button.
  m_btnClear = new PlainToolButton(this);
  m_btnClear->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));
  m_btnClear->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_btnClear->setToolTip(tr("Clear current shortcut."));

  // Clear main shortcut catching button.
  m_shortcutBox = new QKeySequenceEdit(this);
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
  connect(m_shortcutBox, &QKeySequenceEdit::keySequenceChanged, this, &ShortcutCatcher::shortcutChanged);
}

QKeySequence ShortcutCatcher::shortcut() const {
  return m_shortcutBox->keySequence();
}

void ShortcutCatcher::setDefaultShortcut(const QKeySequence& key) {
  m_defaultSequence = key;
  setShortcut(key);
}

void ShortcutCatcher::setShortcut(const QKeySequence& key) {
  m_shortcutBox->setKeySequence(key);
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
