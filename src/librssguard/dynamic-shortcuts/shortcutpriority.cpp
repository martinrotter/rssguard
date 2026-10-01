// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/shortcutpriority.h"

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QShortcutEvent>
#include <QTextEdit>
#include <QThread>
#include <QWindow>

namespace {
  constexpr char INPUT_METHOD_EDITOR_PROPERTY[] = "_rssguard_shortcut_input_method_editor";
}

ShortcutPriority::ShortcutPriority(QWidget* main_window, ActionProvider actions)
  : m_mainWindow(main_window), m_actions(std::move(actions)) {}

QList<QKeySequence> ShortcutPriority::keyCandidates(const QKeyEvent& event) {
  switch (event.key()) {
    case 0:
    case Qt::Key_unknown:
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_AltGr:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
    case Qt::Key_ScrollLock:
      return {};
    default:
      break;
  }

  QList<QKeySequence> candidates;
  const auto append = [&candidates](int key, Qt::KeyboardModifiers modifiers) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QKeySequence sequence(QKeyCombination(modifiers, Qt::Key(key)));
#else
    const QKeySequence sequence(key | int(modifiers));
#endif
    if (!candidates.contains(sequence)) {
      candidates.append(sequence);
    }
  };

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  candidates.append(QKeySequence(event.keyCombination()));
#else
  append(event.key(), event.modifiers());
#endif

  const auto append_variants = [&](Qt::KeyboardModifiers modifiers) {
    append(event.key(), modifiers);

    if (event.key() == Qt::Key_Backtab && modifiers.testFlag(Qt::ShiftModifier)) {
      append(Qt::Key_Tab, modifiers | Qt::ShiftModifier);
    }

    // Native mapping can omit Shift for a symbol the platform already reports
    // as the Qt key. Text alone must not turn a digit into a symbol, and Qt's
    // synthetic fallback has no layout candidates beyond the direct combination.
    // Native scan-code presence is a guard, never a shortcut identity.
    if (event.nativeScanCode() != 0 && modifiers.testFlag(Qt::ShiftModifier) && event.text().size() == 1) {
      const ushort symbol = event.text().at(0).unicode();
      const bool punctuation = (symbol >= '!' && symbol <= '/') || (symbol >= ':' && symbol <= '@') ||
                               (symbol >= '[' && symbol <= '`') || (symbol >= '{' && symbol <= '~');

      if (punctuation && event.key() == symbol && !modifiers.testFlag(Qt::GroupSwitchModifier)) {
        append(symbol, modifiers & ~Qt::ShiftModifier);
      }
    }
  };

  append_variants(event.modifiers());

  if (event.modifiers().testFlag(Qt::KeypadModifier)) {
    append_variants(event.modifiers() & ~Qt::KeypadModifier);
  }

  return candidates;
}

bool ShortcutPriority::sequencesConflict(const QKeySequence& first, const QKeySequence& second) {
  return !first.isEmpty() && !second.isEmpty() &&
         (first.matches(second) != QKeySequence::NoMatch || second.matches(first) != QKeySequence::NoMatch);
}

bool ShortcutPriority::isRecorder(const QObject* object) {
  for (const QObject* ancestor = object; ancestor != nullptr; ancestor = ancestor->parent()) {
    if (qobject_cast<const QKeySequenceEdit*>(ancestor) != nullptr || ancestor->inherits("ShortcutCatcher")) {
      return true;
    }
  }

  return false;
}

bool ShortcutPriority::inMainWindow(QObject* receiver) const {
  if (m_mainWindow.isNull() || !m_mainWindow->isVisible() || !m_mainWindow->isEnabled() ||
      QApplication::activeWindow() != m_mainWindow || QApplication::activeModalWidget() != nullptr ||
      QApplication::activePopupWidget() != nullptr) {
    return false;
  }

  QWidget* focus = QApplication::focusWidget();

  if (focus == nullptr || focus->window() != m_mainWindow || isRecorder(focus) || isRecorder(receiver)) {
    return false;
  }

  if (auto* widget = qobject_cast<QWidget*>(receiver)) {
    return widget->window() == m_mainWindow;
  }

  // Native key delivery starts at the top-level QWindow. Do not include other
  // top-level windows just because their QObject parent belongs to FormMain.
  if (auto* window = qobject_cast<QWindow*>(receiver)) {
    return window == m_mainWindow->windowHandle();
  }

  if (auto* action = qobject_cast<QAction*>(receiver)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return action->associatedObjects().contains(m_mainWindow.data());
#else
    return action->associatedWidgets().contains(m_mainWindow.data());
#endif
  }

  // QShortcutEvent is delivered to QAction/QShortcut, rather than a widget.
  for (QObject* ancestor = receiver->parent(); ancestor != nullptr; ancestor = ancestor->parent()) {
    if (auto* widget = qobject_cast<QWidget*>(ancestor)) {
      return widget->window() == m_mainWindow;
    }
  }

  return false;
}

bool ShortcutPriority::eligible(const QAction* action) const {
  if (action == nullptr || !action->isEnabled() || !action->isVisible() ||
      action->shortcutContext() != Qt::WindowShortcut || m_mainWindow.isNull()) {
    return false;
  }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return action->associatedObjects().contains(m_mainWindow.data());
#else
  return action->associatedWidgets().contains(m_mainWindow.data());
#endif
}

QList<QPointer<QAction>> ShortcutPriority::liveActions() const {
  QList<QPointer<QAction>> result;

  for (QAction* action : m_actions()) {
    if (action != nullptr && !result.contains(action)) {
      result.append(action);
    }
  }

  return result;
}

bool ShortcutPriority::isEditingKey(const QKeyEvent& event) {
  // Use Qt's platform bindings, including alternate clipboard, movement and
  // deletion combinations, rather than assuming Windows Ctrl shortcuts.
  const QKeySequence::StandardKey editing_keys[] = {QKeySequence::Copy,
                                                    QKeySequence::Cut,
                                                    QKeySequence::Paste,
                                                    QKeySequence::Undo,
                                                    QKeySequence::Redo,
                                                    QKeySequence::SelectAll,
                                                    QKeySequence::Deselect,
                                                    QKeySequence::Delete,
                                                    QKeySequence::Backspace,
                                                    QKeySequence::DeleteStartOfWord,
                                                    QKeySequence::DeleteEndOfWord,
                                                    QKeySequence::DeleteEndOfLine,
                                                    QKeySequence::DeleteCompleteLine,
                                                    QKeySequence::InsertParagraphSeparator,
                                                    QKeySequence::InsertLineSeparator,
                                                    QKeySequence::MoveToNextChar,
                                                    QKeySequence::MoveToPreviousChar,
                                                    QKeySequence::MoveToNextWord,
                                                    QKeySequence::MoveToPreviousWord,
                                                    QKeySequence::MoveToNextLine,
                                                    QKeySequence::MoveToPreviousLine,
                                                    QKeySequence::MoveToNextPage,
                                                    QKeySequence::MoveToPreviousPage,
                                                    QKeySequence::MoveToStartOfLine,
                                                    QKeySequence::MoveToEndOfLine,
                                                    QKeySequence::MoveToStartOfBlock,
                                                    QKeySequence::MoveToEndOfBlock,
                                                    QKeySequence::MoveToStartOfDocument,
                                                    QKeySequence::MoveToEndOfDocument,
                                                    QKeySequence::SelectNextChar,
                                                    QKeySequence::SelectPreviousChar,
                                                    QKeySequence::SelectNextWord,
                                                    QKeySequence::SelectPreviousWord,
                                                    QKeySequence::SelectNextLine,
                                                    QKeySequence::SelectPreviousLine,
                                                    QKeySequence::SelectNextPage,
                                                    QKeySequence::SelectPreviousPage,
                                                    QKeySequence::SelectStartOfLine,
                                                    QKeySequence::SelectEndOfLine,
                                                    QKeySequence::SelectStartOfBlock,
                                                    QKeySequence::SelectEndOfBlock,
                                                    QKeySequence::SelectStartOfDocument,
                                                    QKeySequence::SelectEndOfDocument,
                                                    QKeySequence::Cancel};

  for (const auto key : editing_keys) {
    if (event.matches(key)) {
      return true;
    }
  }

  const auto modifiers = event.modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier);
  bool printable_text = false;

  for (const QChar character : event.text()) {
    printable_text |= character.isPrint();
  }

  // AltGr can arrive as GroupSwitch or Ctrl+Alt. Only treat the latter as text
  // entry when the event actually carries printable text.
  bool text_modifiers =
    modifiers == Qt::NoModifier || (!modifiers.testFlag(Qt::MetaModifier) &&
                                    (modifiers.testFlag(Qt::GroupSwitchModifier) ||
                                     (modifiers == (Qt::ControlModifier | Qt::AltModifier) && printable_text)));
#ifdef Q_OS_MACOS
  // Option is also a character/composition modifier on macOS.
  text_modifiers |= modifiers == Qt::AltModifier;
#endif

  if (!text_modifiers) {
    return false;
  }

  if (printable_text || (event.key() >= Qt::Key_Space && event.key() <= 0x10ffff) ||
      (event.key() >= Qt::Key_Dead_Grave && event.key() <= Qt::Key_Dead_Longsolidusoverlay) ||
      event.key() == Qt::Key_Multi_key) {
    return true;
  }

  // Preserve editor submission, indentation/focus navigation and overwrite
  // mode as well as movement keys which a single-line editor may delegate.
  if (modifiers == Qt::NoModifier) {
    switch (event.key()) {
      case Qt::Key_Tab:
      case Qt::Key_Backtab:
      case Qt::Key_Return:
      case Qt::Key_Enter:
      case Qt::Key_Insert:
      case Qt::Key_Backspace:
      case Qt::Key_Delete:
      case Qt::Key_Home:
      case Qt::Key_End:
      case Qt::Key_Left:
      case Qt::Key_Right:
      case Qt::Key_Up:
      case Qt::Key_Down:
      case Qt::Key_PageUp:
      case Qt::Key_PageDown:
        return true;
      default:
        break;
    }
  }

  return false;
}

void ShortcutPriority::registerInputMethodEditor(QWidget* editor) {
  editor->setProperty(INPUT_METHOD_EDITOR_PROPERTY, true);
}

bool ShortcutPriority::hasEditableFocus() {
  QWidget* focus = QApplication::focusWidget();
  bool input_method_editor = false;

  // Include subclasses and internal line edits in combo/spin boxes. Stop at
  // a known read-only editor so a QTextBrowser never becomes a typing context.
  for (QWidget* widget = focus; widget != nullptr; widget = widget->parentWidget()) {
    if (const auto* edit = qobject_cast<QLineEdit*>(widget)) {
      return edit->isEnabled() && !edit->isReadOnly();
    }
    if (const auto* edit = qobject_cast<QTextEdit*>(widget)) {
      return edit->isEnabled() && !edit->isReadOnly();
    }
    if (const auto* edit = qobject_cast<QPlainTextEdit*>(widget)) {
      return edit->isEnabled() && !edit->isReadOnly();
    }
    if (const auto* combo = qobject_cast<QComboBox*>(widget)) {
      return combo->isEnabled() && combo->isEditable() && combo->lineEdit() != nullptr &&
             !combo->lineEdit()->isReadOnly();
    }
    if (const auto* spin = qobject_cast<QAbstractSpinBox*>(widget)) {
      // Qt 5's outer spin-box ImEnabled query can remain true in read-only mode.
      return spin->isEnabled() && !spin->isReadOnly();
    }
    if (qobject_cast<QAbstractItemView*>(widget) != nullptr) {
      // Item views can report ImEnabled merely because the current model item
      // is editable, even with NoEditTriggers. A focused inline editor was
      // recognized above; ordinary view navigation/search is not text editing.
      return false;
    }
    if (widget->property(INPUT_METHOD_EDITOR_PROPERTY).toBool()) {
      input_method_editor = true;
      break;
    }
  }

  // Only explicitly registered editors (including WebEngine) may use delegated
  // input-method state. ImEnabled alone does not identify an active text input.
  if (input_method_editor && focus != nullptr && focus->testAttribute(Qt::WA_InputMethodEnabled)) {
    QInputMethodQueryEvent query(Qt::ImEnabled);
    QCoreApplication::sendEvent(focus, &query);
    return query.value(Qt::ImEnabled).toBool();
  }

  return false;
}

ShortcutPriority::Decision ShortcutPriority::intercept(QObject* receiver, QEvent* event) const {
  if (event->type() != QEvent::ShortcutOverride && event->type() != QEvent::Shortcut) {
    return Decision::PassThrough;
  }

  auto* application = QCoreApplication::instance();

  if (receiver == nullptr || application == nullptr || QThread::currentThread() != application->thread() ||
      receiver->thread() != application->thread() || !inMainWindow(receiver)) {
    return Decision::PassThrough;
  }

  const QPointer<QWidget> main_window(m_mainWindow);
  const QPointer<QObject> target(receiver);
  bool editing_key;

  if (event->type() == QEvent::ShortcutOverride) {
    editing_key = isEditingKey(*static_cast<QKeyEvent*>(event));
  }
  else {
    const auto sequence = static_cast<QShortcutEvent*>(event)->key();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto combination = sequence[0];
    const QKeyEvent key(QEvent::ShortcutOverride, combination.key(), combination.keyboardModifiers());
#else
    const int combination = sequence[0];
    const QKeyEvent key(QEvent::ShortcutOverride,
                        combination & ~Qt::KeyboardModifierMask,
                        Qt::KeyboardModifiers(combination & Qt::KeyboardModifierMask));
#endif
    editing_key = !sequence.isEmpty() && isEditingKey(key);
  }

  const bool editable = editing_key && hasEditableFocus();

  // A public input-method query can run filters and destroy the target/window.
  if (target.isNull() || main_window.isNull()) {
    event->accept();
    return Decision::Consume;
  }

  if (editable) {
    if (event->type() == QEvent::ShortcutOverride) {
      return Decision::DeliverToEditor;
    }

    // Never revive an editing-key assignment through exact ambiguity handling.
    event->accept();
    return Decision::Consume;
  }

  const auto actions = liveActions();

  if (event->type() == QEvent::ShortcutOverride) {
    const auto candidates = keyCandidates(*static_cast<QKeyEvent*>(event));

    for (const auto& action : actions) {
      if (!eligible(action.data())) {
        continue;
      }

      for (const QKeySequence& sequence : action->shortcuts()) {
        for (const QKeySequence& candidate : candidates) {
          if (!sequence.isEmpty() && candidate.matches(sequence) != QKeySequence::NoMatch) {
            // Withhold the override before widgets can prepare editing commands.
            // Qt still owns matching, activation, continuation and autorepeat.
            event->ignore();
            return Decision::Consume;
          }
        }
      }
    }

    return Decision::PassThrough;
  }

  auto* shortcut_event = static_cast<QShortcutEvent*>(event);

  if (!shortcut_event->isAmbiguous()) {
    return Decision::PassThrough;
  }

  QList<QPointer<QAction>> winners;

  for (const auto& action : actions) {
    if (eligible(action.data()) && action->shortcuts().contains(shortcut_event->key())) {
      winners.append(action);
    }
  }

  if (winners.isEmpty()) {
    return Decision::PassThrough;
  }

  // Never let Qt's rotating ambiguous recipient choose a configured winner.
  event->accept();

  if (winners.size() == 1 && eligible(winners.constFirst().data())) {
    winners.constFirst()->trigger();
  }
  else {
    qWarning() << "Conflicting configured shortcut:" << shortcut_event->key().toString()
               << "Resolve the assignments in Settings > Keyboard shortcuts.";
  }

  // Activation can destroy the receiver, main window or this coordinator.
  return Decision::Consume;
}
