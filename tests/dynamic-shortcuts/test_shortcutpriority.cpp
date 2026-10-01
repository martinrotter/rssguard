// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/shortcutpriority.h"
#include "dynamic-shortcuts/shortcutsequenceedit.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QTest>
#include <QTextBrowser>
#include <QTextEdit>
#include <QThread>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWindow>

class PriorityApplication : public QApplication {
  public:
    using QApplication::QApplication;
    ShortcutPriority* priority = nullptr;

    bool notify(QObject* receiver, QEvent* event) override {
      if ((event->type() == QEvent::ShortcutOverride || event->type() == QEvent::Shortcut) &&
          QThread::currentThread() == thread() && priority != nullptr) {
        const auto decision = priority->intercept(receiver, event);
        if (decision == ShortcutPriority::Decision::Consume) {
          return true;
        }
        if (decision == ShortcutPriority::Decision::DeliverToEditor) {
          QApplication::notify(receiver, event);
          event->accept();
          return true;
        }
      }

      return QApplication::notify(receiver, event);
    }
};

class OverrideWidget : public QWidget {
  public:
    explicit OverrideWidget(QWidget* parent = nullptr) : QWidget(parent) {
      setFocusPolicy(Qt::StrongFocus);
    }

    int overrides = 0;
    int presses = 0;
    int releases = 0;
    bool reserveOverrides = true;

  protected:
    bool event(QEvent* event) override {
      if (event->type() == QEvent::ShortcutOverride) {
        ++overrides;
        event->setAccepted(reserveOverrides);
        return reserveOverrides;
      }

      return QWidget::event(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
      ++presses;
      event->accept();
    }

    void keyReleaseEvent(QKeyEvent* event) override {
      ++releases;
      event->accept();
    }
};

class OverrideFilter : public QObject {
  public:
    int overrides = 0;

    bool eventFilter(QObject*, QEvent* event) override {
      if (event->type() == QEvent::ShortcutOverride) {
        ++overrides;
        event->accept();
        return true;
      }

      return false;
    }
};

class InputDelegate : public OverrideWidget {
  public:
    explicit InputDelegate(QWidget* parent) : OverrideWidget(parent) {
      setAttribute(Qt::WA_InputMethodEnabled);
    }
    bool editable = true;
    int queries = 0;

  protected:
    bool event(QEvent* event) override {
      if (event->type() == QEvent::InputMethodQuery) {
        ++queries;
        static_cast<QInputMethodQueryEvent*>(event)->setValue(Qt::ImEnabled, editable);
        return true;
      }
      if (event->type() == QEvent::ShortcutOverride) {
        ++overrides;
        event->ignore(); // Simulate an editor's missing native reservation.
        return false;
      }
      return OverrideWidget::event(event);
    }
};

class PriorityWindow : public QWidget {
  public:
    PriorityWindow()
      : priority(this, [this]() {
          QList<QAction*> result;
          for (const auto& action : registered) {
            if (!action.isNull()) {
              result.append(action.data());
            }
          }
          return result;
        }) {
      layout = new QVBoxLayout(this);
      target = new OverrideWidget(this);
      layout->addWidget(target);
      static_cast<PriorityApplication*>(qApp)->priority = &priority;
      activate(target);
    }

    ~PriorityWindow() override {
      static_cast<PriorityApplication*>(qApp)->priority = nullptr;
    }

    QAction* bind(const QString& sequence) {
      auto* action = new QAction(this);
      action->setShortcut(QKeySequence::fromString(sequence, QKeySequence::PortableText));
      addAction(action);
      registered.append(action);
      return action;
    }

    void activate(QWidget* focus) {
      show();
      QApplication::setActiveWindow(this);
      focus->setFocus();
      QCoreApplication::processEvents();
    }

    ShortcutPriority priority;
    QList<QPointer<QAction>> registered;
    QVBoxLayout* layout;
    OverrideWidget* target;
};

class TestShortcutPriority : public QObject {
    Q_OBJECT

  private slots:
    void candidates();
    void conflicts_data();
    void conflicts();
    void overrideBeforeFilters();
    void symbolAndKeypadActivation();
    void editableFallback();
    void editableTyping_data();
    void editableTyping();
    void standardEditing_data();
    void standardEditing();
    void editorControlKeys();
    void readOnlyInputs_data();
    void readOnlyInputs();
    void editableComboAndSequences();
    void compoundEditorFocus();
    void unregisteredInputMethodWidget();
    void editableDelegateAndComposition_data();
    void editableDelegateAndComposition();
    void itemViewNavigation_data();
    void itemViewNavigation();
    void itemViewSearch_data();
    void itemViewSearch();
    void itemViewEditor_data();
    void itemViewEditor();
    void widgetFallback();
    void liveEligibility();
    void sequencesAndPlayerDelivery();
    void nativeCollision();
    void ambiguousConfiguration();
    void scopeExclusions();
    void tabRecording_data();
    void tabRecording();
    void completeRecording();
    void lifetime();
};

void TestShortcutPriority::candidates() {
  const auto candidates = [](int key, Qt::KeyboardModifiers modifiers, const QString& text = {}) {
    const QKeyEvent event(QEvent::ShortcutOverride, key, modifiers, text);
    return ShortcutPriority::keyCandidates(event);
  };
  const auto sequence = [](const QString& text) {
    return QKeySequence::fromString(text, QKeySequence::PortableText);
  };

  QVERIFY(candidates(Qt::Key_PageDown, Qt::NoModifier).contains(sequence(QStringLiteral("PgDown"))));
  QVERIFY(candidates(Qt::Key_A, Qt::ControlModifier).contains(sequence(QStringLiteral("Ctrl+A"))));
  QVERIFY(candidates(Qt::Key_Enter, Qt::KeypadModifier).contains(sequence(QStringLiteral("Enter"))));
  QVERIFY(candidates(Qt::Key_Backtab, Qt::ShiftModifier).contains(sequence(QStringLiteral("Shift+Tab"))));
  QVERIFY(!candidates(Qt::Key_Backtab, Qt::NoModifier).contains(sequence(QStringLiteral("Shift+Tab"))));
  QVERIFY(!candidates(Qt::Key_1, Qt::ShiftModifier, QStringLiteral("!")).contains(sequence(QStringLiteral("!"))));
  QVERIFY(!candidates(Qt::Key_Plus, Qt::ControlModifier | Qt::ShiftModifier, QStringLiteral("+"))
             .contains(sequence(QStringLiteral("Ctrl++"))));
  const QKeyEvent
    native_symbol(QEvent::ShortcutOverride, Qt::Key_Exclam, Qt::ShiftModifier, 1, 1, 0, QStringLiteral("!"));
  QVERIFY(ShortcutPriority::keyCandidates(native_symbol).contains(sequence(QStringLiteral("!"))));
  const QKeyEvent native_plus(QEvent::ShortcutOverride,
                              Qt::Key_Plus,
                              Qt::ControlModifier | Qt::ShiftModifier,
                              1,
                              1,
                              0,
                              QStringLiteral("+"));
  QVERIFY(ShortcutPriority::keyCandidates(native_plus).contains(sequence(QStringLiteral("Ctrl++"))));
  QVERIFY(!candidates(Qt::Key_A, Qt::ShiftModifier, QStringLiteral("A")).contains(sequence(QStringLiteral("A"))));
  QVERIFY(!candidates(Qt::Key_1, Qt::ShiftModifier, QStringLiteral("ab")).contains(sequence(QStringLiteral("!"))));
  QVERIFY(!candidates(Qt::Key_Q, Qt::ControlModifier | Qt::AltModifier, QStringLiteral("@"))
             .contains(sequence(QStringLiteral("@"))));
  QVERIFY(!candidates(Qt::Key_1, Qt::ShiftModifier | Qt::GroupSwitchModifier, QStringLiteral("!"))
             .contains(sequence(QStringLiteral("!"))));

  for (int key : {0,
                  int(Qt::Key_unknown),
                  int(Qt::Key_Shift),
                  int(Qt::Key_Control),
                  int(Qt::Key_Alt),
                  int(Qt::Key_Meta),
                  int(Qt::Key_AltGr),
                  int(Qt::Key_NumLock),
                  int(Qt::Key_CapsLock),
                  int(Qt::Key_ScrollLock)}) {
    QVERIFY(candidates(key, Qt::NoModifier).isEmpty());
  }

  QCOMPARE(sequence(QStringLiteral("A")).matches(sequence(QStringLiteral("A, B"))), QKeySequence::PartialMatch);
  QCOMPARE(sequence(QStringLiteral("A, B")).matches(sequence(QStringLiteral("A"))), QKeySequence::NoMatch);
}

void TestShortcutPriority::conflicts_data() {
  QTest::addColumn<QString>("first");
  QTest::addColumn<QString>("second");
  QTest::addColumn<bool>("conflict");
  QTest::newRow("duplicate") << "Ctrl+A" << "Ctrl+A" << true;
  QTest::newRow("short-prefix") << "A" << "A, B" << true;
  QTest::newRow("long-prefix") << "A, B" << "A" << true;
  QTest::newRow("nested-prefix") << "A, B" << "A, B, C" << true;
  QTest::newRow("shared-starter") << "A, B" << "A, C" << false;
  QTest::newRow("different") << "A" << "B" << false;
  QTest::newRow("empty-first") << "" << "A" << false;
  QTest::newRow("empty-second") << "A" << "" << false;
}

void TestShortcutPriority::conflicts() {
  QFETCH(QString, first);
  QFETCH(QString, second);
  QFETCH(bool, conflict);
  QCOMPARE(ShortcutPriority::sequencesConflict(QKeySequence(first), QKeySequence(second)), conflict);
}

void TestShortcutPriority::overrideBeforeFilters() {
  PriorityWindow window;
  QAction* action = window.bind(QStringLiteral("PgDown"));
  QSignalSpy triggered(action, &QAction::triggered);
  OverrideFilter filter;
  window.target->installEventFilter(&filter);

  QKeyEvent override_event(QEvent::ShortcutOverride, Qt::Key_PageDown, Qt::NoModifier);
  override_event.accept();
  QVERIFY(QApplication::sendEvent(window.windowHandle(), &override_event));
  QVERIFY(!override_event.isAccepted());

  QTest::keyClick(window.target, Qt::Key_PageDown);
  QCOMPARE(triggered.count(), 1);
  QCOMPARE(filter.overrides, 0);
  QCOMPARE(window.target->overrides, 0);
  QCOMPARE(window.target->presses, 0);

  QTest::keyClick(window.target, Qt::Key_Up);
  QCOMPARE(filter.overrides, 1);
  QCOMPARE(window.target->presses, 1);
}

void TestShortcutPriority::symbolAndKeypadActivation() {
  PriorityWindow window;
  QAction* action = window.bind(QStringLiteral("!"));
  QSignalSpy triggered(action, &QAction::triggered);
  QKeyEvent symbol(QEvent::KeyPress, Qt::Key_1, Qt::ShiftModifier, QStringLiteral("!"));
  QApplication::sendEvent(window.target, &symbol);
  QCOMPARE(triggered.count(), 0);
  QCOMPARE(window.target->presses, 1);
  QCOMPARE(window.target->overrides, 1);

  action->setShortcut(QKeySequence(QStringLiteral("Shift+!")));
  QKeyEvent direct_symbol(QEvent::KeyPress, Qt::Key_Exclam, Qt::ShiftModifier, QStringLiteral("!"));
  QApplication::sendEvent(window.target, &direct_symbol);
  QCOMPARE(triggered.count(), 1);
  QCOMPARE(window.target->presses, 1);

  action->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift++")));
  QKeyEvent plus(QEvent::KeyPress, Qt::Key_Plus, Qt::ControlModifier | Qt::ShiftModifier, QStringLiteral("+"));
  QApplication::sendEvent(window.target, &plus);
  QCOMPARE(triggered.count(), 2);
  QCOMPARE(window.target->presses, 1);

  action->setShortcut(QKeySequence(QStringLiteral("Enter")));
  QKeyEvent keypad(QEvent::KeyPress, Qt::Key_Enter, Qt::KeypadModifier);
  QApplication::sendEvent(window.target, &keypad);
  QCOMPARE(triggered.count(), 3);
  QCOMPARE(window.target->presses, 1);
}

void TestShortcutPriority::editableFallback() {
  PriorityWindow window;
  auto* edit = new QLineEdit(&window);
  window.layout->addWidget(edit);
  window.activate(edit);
  QAction* action = window.bind(QStringLiteral("A"));
  QSignalSpy triggered(action, &QAction::triggered);

  QTest::keyClick(edit, Qt::Key_A);
  QCOMPARE(triggered.count(), 0);
  QCOMPARE(edit->text(), QStringLiteral("a"));
  QTest::keyClick(edit, Qt::Key_U);
  QCOMPARE(edit->text(), QStringLiteral("au"));

  action->setShortcut(QKeySequence(QStringLiteral("Ctrl+A")));
  QTest::keyClick(edit, Qt::Key_A, Qt::ControlModifier);
  QCOMPARE(triggered.count(), 0);
  QCOMPARE(edit->selectedText(), QStringLiteral("au"));
}

namespace {
  QWidget* makeEditor(PriorityWindow& window, int kind) {
    QWidget* editor = kind == 0   ? static_cast<QWidget*>(new QLineEdit(&window))
                      : kind == 1 ? static_cast<QWidget*>(new QPlainTextEdit(&window))
                                  : static_cast<QWidget*>(new QTextEdit(&window));
    window.layout->addWidget(editor);
    if (auto* plain = qobject_cast<QPlainTextEdit*>(editor)) {
      plain->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    else if (auto* rich = qobject_cast<QTextEdit*>(editor)) {
      rich->setLineWrapMode(QTextEdit::NoWrap);
    }
    window.activate(editor);
    return editor;
  }

  QString editorText(QWidget* editor) {
    if (auto* line = qobject_cast<QLineEdit*>(editor)) {
      return line->text();
    }
    if (auto* plain = qobject_cast<QPlainTextEdit*>(editor)) {
      return plain->toPlainText();
    }
    return qobject_cast<QTextEdit*>(editor)->toPlainText();
  }

  void setEditorText(QWidget* editor, const QString& text, int position) {
    if (auto* line = qobject_cast<QLineEdit*>(editor)) {
      line->setText(text);
      line->setCursorPosition(position);
    }
    else if (auto* plain = qobject_cast<QPlainTextEdit*>(editor)) {
      plain->setPlainText(text);
      auto cursor = plain->textCursor();
      cursor.setPosition(position);
      plain->setTextCursor(cursor);
    }
    else {
      auto* rich = qobject_cast<QTextEdit*>(editor);
      rich->setPlainText(text);
      auto cursor = rich->textCursor();
      cursor.setPosition(position);
      rich->setTextCursor(cursor);
    }
  }

  void selectEditorText(QWidget* editor) {
    if (auto* line = qobject_cast<QLineEdit*>(editor)) {
      line->selectAll();
    }
    else if (auto* plain = qobject_cast<QPlainTextEdit*>(editor)) {
      plain->selectAll();
    }
    else {
      qobject_cast<QTextEdit*>(editor)->selectAll();
    }
  }

  QString editorSelection(QWidget* editor) {
    if (auto* line = qobject_cast<QLineEdit*>(editor)) {
      return line->selectedText();
    }
    if (auto* plain = qobject_cast<QPlainTextEdit*>(editor)) {
      return plain->textCursor().selectedText();
    }
    return qobject_cast<QTextEdit*>(editor)->textCursor().selectedText();
  }

  int editorPosition(QWidget* editor) {
    if (auto* line = qobject_cast<QLineEdit*>(editor)) {
      return line->cursorPosition();
    }
    if (auto* plain = qobject_cast<QPlainTextEdit*>(editor)) {
      return plain->textCursor().position();
    }
    return qobject_cast<QTextEdit*>(editor)->textCursor().position();
  }
} // namespace

void TestShortcutPriority::editableTyping_data() {
  QTest::addColumn<int>("kind");
  QTest::addColumn<int>("key");
  QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
  QTest::addColumn<QString>("binding");
  QTest::addColumn<QString>("text");
  for (int kind = 0; kind < 3; ++kind) {
    const auto row = [kind](const char* name) -> QTestData& {
      return QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(kind).arg(QString::fromLatin1(name)))) << kind;
    };
    row("letter") << int(Qt::Key_E) << Qt::KeyboardModifiers(Qt::NoModifier) << QStringLiteral("E")
                  << QStringLiteral("e");
    row("shift") << int(Qt::Key_E) << Qt::KeyboardModifiers(Qt::ShiftModifier) << QStringLiteral("Shift+E")
                 << QStringLiteral("E");
    row("space") << int(Qt::Key_Space) << Qt::KeyboardModifiers(Qt::NoModifier) << QStringLiteral("Space")
                 << QStringLiteral(" ");
    row("symbol") << int(Qt::Key_Exclam) << Qt::KeyboardModifiers(Qt::NoModifier) << QStringLiteral("!")
                  << QStringLiteral("!");
  }
}

void TestShortcutPriority::editableTyping() {
  QFETCH(int, kind);
  QFETCH(int, key);
  QFETCH(Qt::KeyboardModifiers, modifiers);
  QFETCH(QString, binding);
  QFETCH(QString, text);
  PriorityWindow window;
  QWidget* editor = makeEditor(window, kind);
  QAction* action = window.bind(binding);
  QSignalSpy triggered(action, &QAction::triggered);
  // QTest's keyClick uses lowercase text even with Shift. Supply the native
  // text explicitly so this verifies preservation of uppercase input too.
  QKeyEvent press(QEvent::KeyPress, key, modifiers, text);
  QApplication::sendEvent(editor, &press);
  QTest::keyRelease(editor, Qt::Key(key), modifiers);
  QCOMPARE(editorText(editor), text);
  QCOMPARE(triggered.count(), 0);

  action->setShortcut(QKeySequence(QStringLiteral("F5")));
  QTest::keyClick(editor, Qt::Key_F5);
  QCOMPARE(triggered.count(), 1);
  action->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+J")));
  QTest::keyClick(editor, Qt::Key_J, Qt::ControlModifier | Qt::ShiftModifier);
  QCOMPARE(triggered.count(), 2);
  QTest::keyClick(editor, Qt::Key_U);
  QCOMPARE(editorText(editor), text + QStringLiteral("u"));
}

void TestShortcutPriority::standardEditing_data() {
  QTest::addColumn<int>("kind");
  QTest::addColumn<QString>("operation");
  QTest::addColumn<QKeySequence>("binding");
  const QPair<const char*, QKeySequence::StandardKey> operations[] = {
    {"copy", QKeySequence::Copy},
    {"cut", QKeySequence::Cut},
    {"paste", QKeySequence::Paste},
    {"select", QKeySequence::SelectAll},
    {"undo", QKeySequence::Undo},
    {"redo", QKeySequence::Redo},
    {"backspace", QKeySequence::Backspace},
    {"delete", QKeySequence::Delete},
    {"word-backspace", QKeySequence::DeleteStartOfWord},
    {"word-delete", QKeySequence::DeleteEndOfWord},
    {"line-delete", QKeySequence::DeleteEndOfLine},
    {"home", QKeySequence::MoveToStartOfLine},
    {"word-right", QKeySequence::MoveToNextWord},
    {"select-left", QKeySequence::SelectPreviousChar},
    {"page-up", QKeySequence::MoveToPreviousPage},
    {"select-word", QKeySequence::SelectPreviousWord}};
  for (int kind = 0; kind < 3; ++kind) {
    for (const auto& operation : operations) {
      int variant = 0;
      for (const auto& binding : QKeySequence::keyBindings(operation.second)) {
        QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3")
                                   .arg(kind)
                                   .arg(QString::fromLatin1(operation.first))
                                   .arg(variant++)))
          << kind << QString::fromLatin1(operation.first) << binding;
      }
    }
  }
}

void TestShortcutPriority::standardEditing() {
  QFETCH(int, kind);
  QFETCH(QString, operation);
  QFETCH(QKeySequence, binding);
  PriorityWindow window;
  QWidget* editor = makeEditor(window, kind);
  QAction* action = window.bind(binding.toString(QKeySequence::PortableText));
  QSignalSpy triggered(action, &QAction::triggered);
  setEditorText(editor, QStringLiteral("one two"), 7);
  if (operation == QStringLiteral("copy") || operation == QStringLiteral("cut") ||
      operation == QStringLiteral("paste")) {
    selectEditorText(editor);
    QApplication::clipboard()->setText(QStringLiteral("inserted"));
  }
  if (operation == QStringLiteral("delete") || operation == QStringLiteral("word-delete") ||
      operation == QStringLiteral("word-right")) {
    setEditorText(editor, QStringLiteral("one two"), 0);
  }
  if (operation == QStringLiteral("line-delete")) {
    setEditorText(editor, QStringLiteral("one two"), 4);
  }
  if (operation == QStringLiteral("undo") || operation == QStringLiteral("redo")) {
    QTest::keyClick(editor, Qt::Key_X);
    if (operation == QStringLiteral("redo")) {
      QTest::keySequence(editor, QKeySequence(QKeySequence::Undo));
    }
  }

  QTest::keySequence(editor, binding);
  QCOMPARE(triggered.count(), 0);
  if (operation == QStringLiteral("copy")) {
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("one two"));
  }
  else if (operation == QStringLiteral("cut")) {
    QCOMPARE(editorText(editor), QString());
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("one two"));
  }
  else if (operation == QStringLiteral("paste")) {
    QCOMPARE(editorText(editor), QStringLiteral("inserted"));
  }
  else if (operation == QStringLiteral("select")) {
    QCOMPARE(editorSelection(editor), QStringLiteral("one two"));
  }
  else if (operation == QStringLiteral("undo")) {
    QCOMPARE(editorText(editor), QStringLiteral("one two"));
  }
  else if (operation == QStringLiteral("redo")) {
    QCOMPARE(editorText(editor), QStringLiteral("one twox"));
  }
  else if (operation == QStringLiteral("backspace")) {
    QCOMPARE(editorText(editor), QStringLiteral("one tw"));
  }
  else if (operation == QStringLiteral("delete")) {
    QCOMPARE(editorText(editor), QStringLiteral("ne two"));
  }
  else if (operation == QStringLiteral("word-backspace")) {
    QCOMPARE(editorText(editor), QStringLiteral("one "));
  }
  else if (operation == QStringLiteral("word-delete")) {
    QVERIFY(!editorText(editor).contains(QStringLiteral("one")));
    QVERIFY(editorText(editor).contains(QStringLiteral("two")));
  }
  else if (operation == QStringLiteral("line-delete")) {
    QCOMPARE(editorText(editor), QStringLiteral("one "));
  }
  else if (operation == QStringLiteral("home")) {
    QCOMPARE(editorPosition(editor), 0);
  }
  else if (operation == QStringLiteral("word-right")) {
    QVERIFY(editorPosition(editor) > 0);
  }
  else if (operation == QStringLiteral("select-left")) {
    QCOMPARE(editorSelection(editor), QStringLiteral("o"));
  }
  else if (operation == QStringLiteral("select-word")) {
    QCOMPARE(editorSelection(editor), QStringLiteral("two"));
  }
}

void TestShortcutPriority::editorControlKeys() {
  PriorityWindow window;
  for (int kind = 0; kind < 3; ++kind) {
    QWidget* editor = makeEditor(window, kind);
    QAction* action = window.bind(QStringLiteral("Return"));
    QSignalSpy triggered(action, &QAction::triggered);
    if (kind == 0) {
      QSignalSpy submitted(editor, SIGNAL(returnPressed()));
      QTest::keyClick(editor, Qt::Key_Return);
      QCOMPARE(submitted.count(), 1);
    }
    else {
      QTest::keyClick(editor, Qt::Key_Return);
      QCOMPARE(editorText(editor), QStringLiteral("\n"));
    }
    QCOMPARE(triggered.count(), 0);
    action->setShortcut(QKeySequence(QStringLiteral("Tab")));
    QTest::keyClick(editor, Qt::Key_Tab);
    QCOMPARE(triggered.count(), 0);
    if (kind == 0) {
      QVERIFY(QApplication::focusWidget() != editor);
    }
    else {
      QCOMPARE(editorText(editor), QStringLiteral("\n\t"));
    }
    delete action;
  }
}

void TestShortcutPriority::readOnlyInputs_data() {
  QTest::addColumn<int>("kind");
  for (int kind = 0; kind < 4; ++kind) {
    QTest::newRow(qPrintable(QString::number(kind))) << kind;
  }
}

void TestShortcutPriority::readOnlyInputs() {
  QFETCH(int, kind);
  PriorityWindow window;
  QWidget* editor;
  if (kind == 3) {
    editor = new QTextBrowser(&window);
    window.layout->addWidget(editor);
  }
  else {
    editor = makeEditor(window, kind);
    if (auto* line = qobject_cast<QLineEdit*>(editor)) {
      line->setReadOnly(true);
    }
    else if (auto* plain = qobject_cast<QPlainTextEdit*>(editor)) {
      plain->setReadOnly(true);
    }
    else {
      qobject_cast<QTextEdit*>(editor)->setReadOnly(true);
    }
  }
  setEditorText(editor, QStringLiteral("original"), 0);
  window.activate(editor);
  QAction* action = window.bind(QStringLiteral("E"));
  QSignalSpy triggered(action, &QAction::triggered);
  QTest::keyClick(editor, Qt::Key_E);
  QCOMPARE(triggered.count(), 1);
  action->setShortcut(QKeySequence(QStringLiteral("PgDown")));
  QTest::keyClick(editor, Qt::Key_PageDown);
  QCOMPARE(triggered.count(), 2);
  QCOMPARE(editorText(editor), QStringLiteral("original"));
}

void TestShortcutPriority::editableComboAndSequences() {
  PriorityWindow window;
  auto* combo = new QComboBox(&window);
  combo->setEditable(true);
  window.layout->addWidget(combo);
  window.activate(combo->lineEdit());
  QAction* action = window.bind(QStringLiteral("E, B"));
  QSignalSpy triggered(action, &QAction::triggered);
  QTest::keyClick(combo->lineEdit(), Qt::Key_E);
  QTest::keyClick(combo->lineEdit(), Qt::Key_B);
  QCOMPARE(combo->currentText(), QStringLiteral("eb"));
  QCOMPARE(triggered.count(), 0);

  const auto editing_bindings = QKeySequence::keyBindings(QKeySequence::DeleteEndOfLine);
  if (!editing_bindings.isEmpty()) {
    const QKeySequence start = editing_bindings.constFirst();
    action->setShortcut(QKeySequence(start.toString(QKeySequence::PortableText) + QStringLiteral(", S")));
    combo->lineEdit()->setText(QStringLiteral("text"));
    combo->lineEdit()->setCursorPosition(2);
    QTest::keySequence(combo->lineEdit(), start);
    QTest::keyClick(combo->lineEdit(), Qt::Key_S);
    QCOMPARE(combo->currentText(), QStringLiteral("tes"));
    QCOMPARE(triggered.count(), 0);
  }
  window.activate(window.target);
  action->setShortcut(QKeySequence(QStringLiteral("E")));
  QTest::keyClick(window.target, Qt::Key_E);
  QCOMPARE(triggered.count(), 1);
}

void TestShortcutPriority::unregisteredInputMethodWidget() {
  PriorityWindow window;
  auto* widget = new InputDelegate(&window);
  window.layout->addWidget(widget);
  window.activate(widget);
  QAction* action = window.bind(QStringLiteral("E"));
  QSignalSpy triggered(action, &QAction::triggered);
  QTest::keyClick(widget, Qt::Key_E);
  QCOMPARE(triggered.count(), 1);
  QCOMPARE(widget->presses, 0);
  QCOMPARE(widget->queries, 0);

  action->setShortcut(QKeySequence());
  QTest::keyClick(widget, Qt::Key_E);
  QCOMPARE(triggered.count(), 1);
  QCOMPARE(widget->presses, 1);
}

void TestShortcutPriority::editableDelegateAndComposition_data() {
  QTest::addColumn<bool>("register_parent");
  QTest::newRow("focused-editor") << false;
  QTest::newRow("delegated-editor") << true;
}

void TestShortcutPriority::editableDelegateAndComposition() {
  QFETCH(bool, register_parent);
  PriorityWindow window;
  auto* container = new QWidget(&window);
  auto* layout = new QVBoxLayout(container);
  auto* delegate = new InputDelegate(container);
  layout->addWidget(delegate);
  window.layout->addWidget(container);
  ShortcutPriority::registerInputMethodEditor(register_parent ? container : delegate);
  window.activate(delegate);
  QAction* action = window.bind(QStringLiteral("E"));
  QSignalSpy triggered(action, &QAction::triggered);
  QTest::keyClick(delegate, Qt::Key_E);
  QCOMPARE(triggered.count(), 0);
  QCOMPARE(delegate->presses, 1);
  QVERIFY(delegate->overrides > 0); // Native preparation still receives the override.
  QVERIFY(delegate->queries > 0);

  QShortcutEvent ambiguous(QKeySequence(QStringLiteral("E")), 1, true);
  QApplication::sendEvent(action, &ambiguous);
  QCOMPARE(triggered.count(), 0);
  delegate->editable = false;
  QTest::keyClick(delegate, Qt::Key_E);
  QCOMPARE(triggered.count(), 1);
  QCOMPARE(delegate->presses, 1);

  QWidget* editor = makeEditor(window, 0);
  QKeyEvent altgr(QEvent::ShortcutOverride, Qt::Key_E, Qt::ControlModifier | Qt::AltModifier, QStringLiteral("\u20ac"));
  QCOMPARE(window.priority.intercept(editor, &altgr), ShortcutPriority::Decision::DeliverToEditor);
  QKeyEvent dead(QEvent::ShortcutOverride, Qt::Key_Dead_Acute, Qt::NoModifier);
  QCOMPARE(window.priority.intercept(editor, &dead), ShortcutPriority::Decision::DeliverToEditor);
  QInputMethodEvent composition;
  composition.setCommitString(QStringLiteral("\u00e9"));
  QApplication::sendEvent(editor, &composition);
  QCOMPARE(editorText(editor), QStringLiteral("\u00e9"));
  QCOMPARE(triggered.count(), 1);
}

static void itemViewTypes() {
  QTest::addColumn<int>("type");
  QTest::newRow("tree") << 0;
  QTest::newRow("list") << 1;
  QTest::newRow("table") << 2;
}

static QAbstractItemView* makeItemView(PriorityWindow& window, int type) {
  QAbstractItemView* view;
  switch (type) {
    case 0:
      view = new QTreeView(&window);
      break;
    case 1:
      view = new QListView(&window);
      break;
    default:
      view = new QTableView(&window);
      break;
  }
  auto* model = new QStandardItemModel(view);
  for (int row = 0; row < 80; ++row) {
    const QString text = row == 0   ? QStringLiteral("Alpha")
                         : row == 1 ? QStringLiteral("Walnut")
                         : row == 2 ? QStringLiteral("Willow")
                                    : QStringLiteral("Zebra %1").arg(row);
    model->setItem(row, 0, new QStandardItem(text));
  }
  view->setModel(model);
  // Match MessagesView: editable model flags, but no inline editor is open.
  view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  window.layout->addWidget(view);
  window.resize(400, 260);
  view->setCurrentIndex(model->index(0, 0));
  window.activate(view);
  return view;
}

void TestShortcutPriority::itemViewNavigation_data() {
  itemViewTypes();
}

void TestShortcutPriority::itemViewNavigation() {
  QFETCH(int, type);
  PriorityWindow window;
  auto* view = makeItemView(window, type);
  QCOMPARE(QApplication::focusWidget(), view);
  QVERIFY(view->currentIndex().flags().testFlag(Qt::ItemIsEditable));
  QVERIFY(view->testAttribute(Qt::WA_InputMethodEnabled));
  QInputMethodQueryEvent query(Qt::ImEnabled);
  QApplication::sendEvent(view, &query);
  QVERIFY(query.value(Qt::ImEnabled).toBool());

  QAction* down = window.bind(QStringLiteral("PgDown"));
  QAction* up = window.bind(QStringLiteral("PgUp"));
  QSignalSpy down_triggered(down, &QAction::triggered);
  QSignalSpy up_triggered(up, &QAction::triggered);
  const QModelIndex current = view->currentIndex();
  const int position = view->verticalScrollBar()->value();
  QTest::keyClick(view, Qt::Key_PageDown);
  QTest::keyClick(view, Qt::Key_PageUp);
  QCOMPARE(down_triggered.count(), 1);
  QCOMPARE(up_triggered.count(), 1);
  QCOMPARE(view->currentIndex(), current);
  QCOMPARE(view->verticalScrollBar()->value(), position);

  down->setShortcut(QKeySequence());
  QTest::keyClick(view, Qt::Key_PageDown);
  QTest::keyClick(view, Qt::Key_PageDown);
  QCOMPARE(down_triggered.count(), 1);
  QVERIFY(view->currentIndex().row() > current.row());
  QVERIFY(view->verticalScrollBar()->value() > position);
}

void TestShortcutPriority::itemViewSearch_data() {
  itemViewTypes();
}

void TestShortcutPriority::itemViewSearch() {
  QFETCH(int, type);
  int native_first;
  int native_second;
  {
    PriorityWindow native_window;
    auto* native_view = makeItemView(native_window, type);
    static_cast<PriorityApplication*>(qApp)->priority = nullptr;
    QTest::keyClick(native_view, Qt::Key_W);
    native_first = native_view->currentIndex().row();
    QTest::keyClick(native_view, Qt::Key_W);
    native_second = native_view->currentIndex().row();
  }
  QCOMPARE(native_first, 1);
  if (type == 0) {
    QCOMPARE(native_second, 2); // The article list's QTreeView letter cycling.
  }

  PriorityWindow window;
  auto* view = makeItemView(window, type);
  QAction* action = window.bind(QStringLiteral("W"));
  QSignalSpy triggered(action, &QAction::triggered);
  QTest::keyClick(view, Qt::Key_W);
  QTest::keyClick(view, Qt::Key_W);
  QCOMPARE(triggered.count(), 2);
  QCOMPARE(view->currentIndex().row(), 0);

  action->setEnabled(false);
  QTest::keyClick(view, Qt::Key_W);
  QCOMPARE(view->currentIndex().row(), native_first);
  QTest::keyClick(view, Qt::Key_W);
  // Preserve each view's own search semantics, including repeated letters.
  QCOMPARE(view->currentIndex().row(), native_second);
  QCOMPARE(triggered.count(), 2);

  action->setEnabled(true);
  action->setShortcut(QKeySequence());
  view->keyboardSearch(QString()); // Start a fresh native search for this phase.
  view->setCurrentIndex(view->model()->index(0, 0));
  QTest::keyClick(view, Qt::Key_W);
  QCOMPARE(view->currentIndex().row(), 1);
  QCOMPARE(triggered.count(), 2);

  // A configured sequence prefix also takes precedence over letter search.
  action->setShortcut(QKeySequence(QStringLiteral("W, X")));
  view->setCurrentIndex(view->model()->index(0, 0));
  QTest::keyClick(view, Qt::Key_W);
  QCOMPARE(view->currentIndex().row(), 0);
  QTest::keyClick(view, Qt::Key_X);
  QCOMPARE(triggered.count(), 3);
  QCOMPARE(view->currentIndex().row(), 0);
}

void TestShortcutPriority::itemViewEditor_data() {
  itemViewTypes();
}

void TestShortcutPriority::itemViewEditor() {
  QFETCH(int, type);
  PriorityWindow window;
  auto* view = makeItemView(window, type);
  view->setEditTriggers(QAbstractItemView::EditKeyPressed);
  view->edit(view->currentIndex());
  QTRY_VERIFY(qobject_cast<QLineEdit*>(QApplication::focusWidget()) != nullptr);
  auto* editor = qobject_cast<QLineEdit*>(QApplication::focusWidget());
  QVERIFY(view->isAncestorOf(editor));

  QAction* letter = window.bind(QStringLiteral("W"));
  QAction* select_all = window.bind(QStringLiteral("Ctrl+A"));
  QAction* copy = window.bind(QStringLiteral("Ctrl+C"));
  QAction* unrelated = window.bind(QStringLiteral("F5"));
  QSignalSpy letter_triggered(letter, &QAction::triggered);
  QSignalSpy select_triggered(select_all, &QAction::triggered);
  QSignalSpy copy_triggered(copy, &QAction::triggered);
  QSignalSpy unrelated_triggered(unrelated, &QAction::triggered);
  editor->selectAll();
  QTest::keyClick(editor, Qt::Key_W);
  QCOMPARE(editor->text(), QStringLiteral("w"));
  QTest::keyClick(editor, Qt::Key_A, Qt::ControlModifier);
  QCOMPARE(editor->selectedText(), QStringLiteral("w"));
  QApplication::clipboard()->clear();
  QTest::keyClick(editor, Qt::Key_C, Qt::ControlModifier);
  QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("w"));
  QCOMPARE(letter_triggered.count(), 0);
  QCOMPARE(select_triggered.count(), 0);
  QCOMPARE(copy_triggered.count(), 0);
  QTest::keyClick(editor, Qt::Key_F5);
  QCOMPARE(unrelated_triggered.count(), 1);

  QTest::keyClick(editor, Qt::Key_Escape);
  QTRY_COMPARE(QApplication::focusWidget(), view);
  QTest::keyClick(view, Qt::Key_W);
  QCOMPARE(letter_triggered.count(), 1);
}

void TestShortcutPriority::compoundEditorFocus() {
  PriorityWindow window;
  auto* combo = new QComboBox(&window);
  combo->setEditable(true);
  window.layout->addWidget(combo);
  window.activate(combo);
  QAction* action = window.bind(QStringLiteral("E"));
  QSignalSpy triggered(action, &QAction::triggered);
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_E);
  QCOMPARE(combo->currentText(), QStringLiteral("e"));
  QCOMPARE(triggered.count(), 0);
  combo->setEditable(false);
  window.activate(combo);
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_E);
  QCOMPARE(triggered.count(), 1);
  combo->setEditable(true);
  combo->lineEdit()->setReadOnly(true);
  window.activate(combo);
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_E);
  QCOMPARE(triggered.count(), 2);

  auto* spin = new QSpinBox(&window);
  spin->setRange(0, 9999);
  spin->setValue(10);
  window.layout->addWidget(spin);
  window.activate(spin);
  auto* internal = spin->findChild<QLineEdit*>();
  QVERIFY(internal != nullptr);
  internal->deselect();
  internal->setCursorPosition(internal->text().size());
  action->setShortcut(QKeySequence(QStringLiteral("Backspace")));
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Backspace);
  QCOMPARE(spin->value(), 1);
  QCOMPARE(triggered.count(), 2);
  spin->setReadOnly(true);
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_Backspace);
  QCOMPARE(triggered.count(), 3);
  QCOMPARE(spin->value(), 1);
}

void TestShortcutPriority::widgetFallback() {
  PriorityWindow window;
  window.target->reserveOverrides = false;
  QAction* action = window.bind(QStringLiteral("E"));
  QSignalSpy application_triggered(action, &QAction::triggered);
  QShortcut local(QKeySequence(QStringLiteral("F6")), window.target);
  QSignalSpy local_triggered(&local, &QShortcut::activated);

  QTest::keyClick(window.target, Qt::Key_E);
  QCOMPARE(application_triggered.count(), 1);
  QCOMPARE(window.target->presses, 0);
  QTest::keyClick(window.target, Qt::Key_F6);
  QCOMPARE(local_triggered.count(), 1);
  action->setShortcut({});
  QTest::keyClick(window.target, Qt::Key_E);
  QTest::keyClick(window.target, Qt::Key_U);
  QCOMPARE(window.target->presses, 2);
  QCOMPARE(application_triggered.count(), 1);
}

void TestShortcutPriority::liveEligibility() {
  PriorityWindow window;
  QAction* action = window.bind(QStringLiteral("Space"));
  QSignalSpy triggered(action, &QAction::triggered);
  const auto reserved = [&]() {
    QKeyEvent event(QEvent::ShortcutOverride, Qt::Key_Space, Qt::NoModifier);
    return window.priority.intercept(window.target, &event) == ShortcutPriority::Decision::Consume;
  };

  QVERIFY(reserved());
  action->setEnabled(false);
  QVERIFY(!reserved());
  action->setEnabled(true);
  action->setVisible(false);
  QVERIFY(!reserved());
  action->setVisible(true);
  action->setShortcut({});
  QVERIFY(!reserved());
  action->setShortcuts({QKeySequence(QStringLiteral("A")), QKeySequence(QStringLiteral("Space"))});
  QVERIFY(reserved());
  action->setShortcutContext(Qt::ApplicationShortcut);
  QVERIFY(!reserved());
  action->setShortcutContext(Qt::WindowShortcut);
  window.removeAction(action);
  QVERIFY(!reserved());
  window.addAction(action);
  QVERIFY(reserved());
  window.registered.append(action); // Duplicate discovery must not invent ambiguity.
  QTest::keyClick(window.target, Qt::Key_Space);
  QCOMPARE(triggered.count(), 1);
  delete action;
  QVERIFY(!reserved());
}

void TestShortcutPriority::sequencesAndPlayerDelivery() {
  PriorityWindow window;
  QAction* first = window.bind(QStringLiteral("A, B"));
  QAction* second = window.bind(QStringLiteral("A, C"));
  QSignalSpy first_triggered(first, &QAction::triggered);
  QSignalSpy second_triggered(second, &QAction::triggered);

  QTest::keyClick(window.target, Qt::Key_A);
  QTest::keyClick(window.target, Qt::Key_B);
  QCOMPARE(first_triggered.count(), 1);
  QCOMPARE(second_triggered.count(), 0);
  QCOMPARE(window.target->presses, 0);
  QVERIFY(window.target->releases >= 2); // Releases still arrive; MPV must ignore them.

  QTest::keyClick(window.target, Qt::Key_A);
  QTest::keyClick(window.target, Qt::Key_C);
  QCOMPARE(second_triggered.count(), 1);
  QCOMPARE(window.target->presses, 0);
  QTest::keyClick(window.target, Qt::Key_Space);
  QCOMPARE(window.target->presses, 1);

  QAction* repeat = window.bind(QStringLiteral("Down"));
  QSignalSpy repeated(repeat, &QAction::triggered);
  QKeyEvent repeated_press(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, {}, true);
  QApplication::sendEvent(window.target, &repeated_press);
  QCOMPARE(repeated.count(), 1);
  QCOMPARE(window.target->presses, 1);
  repeat->setAutoRepeat(false);
  QApplication::sendEvent(window.target, &repeated_press);
  QCOMPARE(repeated.count(), 1);
  QCOMPARE(window.target->presses, 1);
}

void TestShortcutPriority::nativeCollision() {
  PriorityWindow window;
  QAction* action = window.bind(QStringLiteral("Space"));
  action->setCheckable(true);
  QShortcut local(QKeySequence(QStringLiteral("Space")), window.target);
  QSignalSpy triggered(action, &QAction::triggered);
  QSignalSpy local_triggered(&local, &QShortcut::activated);
  QSignalSpy local_ambiguous(&local, &QShortcut::activatedAmbiguously);

  QTest::keyClick(window.target, Qt::Key_Space);
  QCOMPARE(triggered.count(), 1);
  QVERIFY(action->isChecked());
  QTest::keyClick(window.target, Qt::Key_Space);
  QCOMPARE(triggered.count(), 2);
  QVERIFY(!action->isChecked());
  QCOMPARE(local_triggered.count(), 0);
  QCOMPARE(local_ambiguous.count(), 0);
  QCOMPARE(window.target->presses, 0);
}

void TestShortcutPriority::ambiguousConfiguration() {
  PriorityWindow window;
  QAction* first = window.bind(QStringLiteral("Space"));
  QAction* second = window.bind(QStringLiteral("Space"));
  QSignalSpy first_triggered(first, &QAction::triggered);
  QSignalSpy second_triggered(second, &QAction::triggered);
  QTest::keyClick(window.target, Qt::Key_Space);
  QCOMPARE(first_triggered.count(), 0);
  QCOMPARE(second_triggered.count(), 0);
  QCOMPARE(window.target->presses, 0);
}

void TestShortcutPriority::scopeExclusions() {
  PriorityWindow window;
  QAction* action = window.bind(QStringLiteral("A"));
  QSignalSpy triggered(action, &QAction::triggered);
  auto* recorder = new ShortcutSequenceEdit(&window);
  window.layout->addWidget(recorder);
  window.activate(recorder);
  QTest::keyClick(recorder, Qt::Key_A);
  QTRY_COMPARE(recorder->keySequence(), QKeySequence(QStringLiteral("A")));
  QCOMPARE(triggered.count(), 0);

  QDialog dialog(&window);
  auto* edit = new QLineEdit(&dialog);
  dialog.show();
  QApplication::setActiveWindow(&dialog);
  edit->setFocus();
  QTest::keyClick(edit, Qt::Key_A);
  QCOMPARE(edit->text(), QStringLiteral("a"));
  QCOMPARE(triggered.count(), 0);
  dialog.hide();
  window.activate(window.target);

  QMenu popup(&window);
  popup.addAction(QStringLiteral("Example"));
  popup.popup(QPoint(0, 0));
  QKeyEvent event(QEvent::ShortcutOverride, Qt::Key_A, Qt::NoModifier);
  QCOMPARE(window.priority.intercept(window.target, &event), ShortcutPriority::Decision::PassThrough);
  popup.close();

  QWidget tool(&window, Qt::Tool);
  tool.show();
  QApplication::setActiveWindow(&tool);
  QCOMPARE(window.priority.intercept(&tool, &event), ShortcutPriority::Decision::PassThrough);
  QCOMPARE(window.priority.intercept(tool.windowHandle(), &event), ShortcutPriority::Decision::PassThrough);
}

void TestShortcutPriority::tabRecording_data() {
  QTest::addColumn<int>("key");
  QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
  QTest::newRow("tab") << int(Qt::Key_Tab) << Qt::KeyboardModifiers(Qt::NoModifier);
  QTest::newRow("backtab") << int(Qt::Key_Backtab) << Qt::KeyboardModifiers(Qt::ShiftModifier);
}

void TestShortcutPriority::tabRecording() {
  QFETCH(int, key);
  QFETCH(Qt::KeyboardModifiers, modifiers);
  PriorityWindow window;
  auto* recorder = new ShortcutSequenceEdit(&window);
  window.layout->addWidget(recorder);
  window.activate(recorder);
  QSignalSpy finished(recorder, &QKeySequenceEdit::editingFinished);
  QTest::keyClick(recorder, Qt::Key(key), modifiers);
  QTRY_VERIFY(!finished.isEmpty());
  QVERIFY(!recorder->keySequence().isEmpty());
  QVERIFY(QApplication::focusWidget() == recorder || recorder->isAncestorOf(QApplication::focusWidget()));
  const QKeyEvent event(QEvent::ShortcutOverride, key, modifiers);
  QVERIFY(ShortcutPriority::keyCandidates(event).contains(recorder->keySequence()));

  QAction* action = window.bind({});
  action->setShortcut(recorder->keySequence());
  QSignalSpy triggered(action, &QAction::triggered);
  window.activate(window.target);
  QTest::keyClick(window.target, Qt::Key(key), modifiers);
  QCOMPARE(triggered.count(), 1);
  QCOMPARE(window.target->presses, modifiers.testFlag(Qt::ShiftModifier) ? 1 : 0);
}

void TestShortcutPriority::completeRecording() {
  PriorityWindow window;
  auto* recorder = new ShortcutSequenceEdit(&window);
  window.layout->addWidget(recorder);
  window.activate(recorder);
  QSignalSpy finished(recorder, &QKeySequenceEdit::editingFinished);
  for (Qt::Key key : {Qt::Key_A, Qt::Key_B, Qt::Key_C, Qt::Key_D}) {
    QTest::keyClick(recorder, key);
  }
  QTRY_VERIFY(!finished.isEmpty());
  QCOMPARE(recorder->keySequence(), QKeySequence(QStringLiteral("A, B, C, D")));
}

void TestShortcutPriority::lifetime() {
  auto* main_window = new QWidget();
  ShortcutPriority priority(main_window, []() {
    return QList<QAction*>();
  });
  delete main_window;
  QWidget target;
  QKeyEvent event(QEvent::ShortcutOverride, Qt::Key_A, Qt::NoModifier);
  QCOMPARE(priority.intercept(&target, &event), ShortcutPriority::Decision::PassThrough);

  PriorityWindow window;
  QAction* action = window.bind(QStringLiteral("Space"));
  new QShortcut(QKeySequence(QStringLiteral("Space")), window.target);
  QPointer<QWidget> watched(window.target);
  connect(action, &QAction::triggered, &window, [&]() {
    delete window.target;
  });
  QTest::keyPress(window.target, Qt::Key_Space);
  QVERIFY(watched.isNull());
}

int main(int argc, char** argv) {
  PriorityApplication application(argc, argv);
  application.setStyle(QStringLiteral("Fusion"));
  TestShortcutPriority test;
  return QTest::qExec(&test, argc, argv);
}

#include "test_shortcutpriority.moc"
