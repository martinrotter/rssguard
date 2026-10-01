// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/dynamicshortcutswidget.h"
#include "dynamic-shortcuts/shortcutcatcher.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"

#include <QAction>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QSignalSpy>
#include <QTest>

class TestShortcutSettings : public QObject {
    Q_OBJECT

  private slots:
    void init();
    void decliningRestoresPrevious();
    void replacesEveryPrefixConflict();
    void cancelUndoAndClear();
    void loadedDuplicatesAndRepopulation();
    void completedSequenceOnly();
    void applyFinishesRecording();
    void destructionDuringPrompt();
    void destroyedAction();

  private:
    int m_prompts = 0;
    QString m_details;
    QMessageBox::StandardButton m_decision = QMessageBox::Yes;

    static ShortcutCatcher* catcher(DynamicShortcutsWidget& widget, QAction& action);
    static void bind(QAction& action, const QString& name, const QString& sequence);
};

void TestShortcutSettings::init() {
  m_prompts = 0;
  m_details.clear();
  m_decision = QMessageBox::Yes;
  MsgBox::decision = [this](QWidget*, const QString& details) {
    ++m_prompts;
    m_details = details;
    return m_decision;
  };
}

ShortcutCatcher* TestShortcutSettings::catcher(DynamicShortcutsWidget& widget, QAction& action) {
  for (auto* binding : widget.findChildren<ShortcutCatcher*>()) {
    if (binding->action() == &action) {
      return binding;
    }
  }
  return nullptr;
}

void TestShortcutSettings::bind(QAction& action, const QString& name, const QString& sequence) {
  action.setText(name);
  action.setShortcut(QKeySequence(sequence));
}

void TestShortcutSettings::decliningRestoresPrevious() {
  QAction first, edited;
  bind(first, QStringLiteral("First"), QStringLiteral("A, B"));
  bind(edited, QStringLiteral("Edited"), QStringLiteral("Z"));
  DynamicShortcutsWidget widget;
  widget.populate({&first, &edited});
  QSignalSpy changed(&widget, &DynamicShortcutsWidget::setupChanged);
  m_decision = QMessageBox::No;

  catcher(widget, edited)->setShortcut(QKeySequence(QStringLiteral("A")));
  QCOMPARE(m_prompts, 1);
  QVERIFY(m_details.contains(QStringLiteral("First")));
  QCOMPARE(catcher(widget, edited)->shortcut(), QKeySequence(QStringLiteral("Z")));
  QCOMPARE(catcher(widget, first)->shortcut(), QKeySequence(QStringLiteral("A, B")));
  QCOMPARE(changed.count(), 0);
  widget.updateShortcuts();
  QCOMPARE(edited.shortcut(), QKeySequence(QStringLiteral("Z")));
  QCOMPARE(first.shortcut(), QKeySequence(QStringLiteral("A, B")));
}

void TestShortcutSettings::replacesEveryPrefixConflict() {
  QAction first, second, edited;
  bind(first, QStringLiteral("First"), QStringLiteral("A, B"));
  bind(second, QStringLiteral("Second"), QStringLiteral("A, C"));
  bind(edited, QStringLiteral("Edited"), QStringLiteral("Z"));
  DynamicShortcutsWidget widget;
  widget.populate({&first, &second, &edited});
  QSignalSpy changed(&widget, &DynamicShortcutsWidget::setupChanged);

  catcher(widget, edited)->setShortcut(QKeySequence(QStringLiteral("A")));
  QCOMPARE(m_prompts, 1);
  QVERIFY(m_details.contains(QStringLiteral("First")));
  QVERIFY(m_details.contains(QStringLiteral("Second")));
  QVERIFY(catcher(widget, first)->shortcut().isEmpty());
  QVERIFY(catcher(widget, second)->shortcut().isEmpty());
  QCOMPARE(changed.count(), 1);
  QCOMPARE(first.shortcut(), QKeySequence(QStringLiteral("A, B")));
  QCOMPARE(second.shortcut(), QKeySequence(QStringLiteral("A, C")));
  QCOMPARE(edited.shortcut(), QKeySequence(QStringLiteral("Z")));

  widget.updateShortcuts();
  QVERIFY(first.shortcut().isEmpty());
  QVERIFY(second.shortcut().isEmpty());
  QCOMPARE(edited.shortcut(), QKeySequence(QStringLiteral("A")));
}

void TestShortcutSettings::cancelUndoAndClear() {
  QAction first, second;
  bind(first, QStringLiteral("First"), QStringLiteral("A"));
  bind(second, QStringLiteral("Second"), QStringLiteral("B"));
  {
    DynamicShortcutsWidget widget;
    widget.populate({&first, &second});
    auto* edited = catcher(widget, second);
    edited->setShortcut(QKeySequence(QStringLiteral("C")));
    edited->resetShortcut();
    QCOMPARE(edited->shortcut(), QKeySequence(QStringLiteral("B")));
    edited->clearShortcut();
    catcher(widget, first)->setShortcut(QKeySequence(QStringLiteral("B")));
    QCOMPARE(m_prompts, 0); // Clearing removed the old binding from staged conflict checks.
  }
  QCOMPARE(first.shortcut(), QKeySequence(QStringLiteral("A")));
  QCOMPARE(second.shortcut(), QKeySequence(QStringLiteral("B")));
}

void TestShortcutSettings::loadedDuplicatesAndRepopulation() {
  QAction first, second;
  bind(first, QStringLiteral("First"), QStringLiteral("A"));
  bind(second, QStringLiteral("Second"), QStringLiteral("A"));
  DynamicShortcutsWidget widget;
  widget.populate({&first, &second});
  QCOMPARE(m_prompts, 0);
  bool warned = false;
  for (auto* label : widget.findChildren<QLabel*>()) {
    if (!label->isHidden() && label->text().contains(QStringLiteral("First (A) conflicts with Second (A)"))) {
      warned = true;
    }
  }
  QVERIFY(warned);
  QPointer<ShortcutCatcher> previous(catcher(widget, first));
  widget.populate({&first});
  QVERIFY(previous.isNull());
  QCOMPARE(widget.findChildren<ShortcutCatcher*>().size(), 1);
  QCOMPARE(first.shortcut(), QKeySequence(QStringLiteral("A")));
}

void TestShortcutSettings::completedSequenceOnly() {
  QAction first, edited;
  bind(first, QStringLiteral("First"), QStringLiteral("A, B"));
  bind(edited, QStringLiteral("Edited"), QStringLiteral("Z"));
  DynamicShortcutsWidget widget;
  widget.populate({&first, &edited});
  widget.show();
  QApplication::setActiveWindow(&widget);
  auto* binding = catcher(widget, edited);
  auto* recorder = binding->findChild<QKeySequenceEdit*>();
  recorder->setFocus();
  QSignalSpy changed(&widget, &DynamicShortcutsWidget::setupChanged);

  QTest::keyClick(recorder, Qt::Key_A);
  QCOMPARE(binding->shortcut(), QKeySequence(QStringLiteral("Z")));
  QCOMPARE(m_prompts, 0);
  QTest::keyClick(recorder, Qt::Key_C);
  QTRY_COMPARE(binding->shortcut(), QKeySequence(QStringLiteral("A, C")));
  QCOMPARE(m_prompts, 0);
  QCOMPARE(changed.count(), 1);
  QCOMPARE(edited.shortcut(), QKeySequence(QStringLiteral("Z")));
}

void TestShortcutSettings::applyFinishesRecording() {
  QAction edited;
  bind(edited, QStringLiteral("Edited"), QStringLiteral("Z"));
  DynamicShortcutsWidget widget;
  widget.populate({&edited});
  widget.show();
  QApplication::setActiveWindow(&widget);
  auto* binding = catcher(widget, edited);
  auto* recorder = binding->findChild<QKeySequenceEdit*>();
  recorder->setFocus();
  QTest::keyClick(recorder, Qt::Key_D);
  QCOMPARE(edited.shortcut(), QKeySequence(QStringLiteral("Z")));
  widget.updateShortcuts();
  QCOMPARE(edited.shortcut(), QKeySequence(QStringLiteral("D")));
  QCOMPARE(m_prompts, 0);
}

void TestShortcutSettings::destructionDuringPrompt() {
  QAction first, edited;
  bind(first, QStringLiteral("First"), QStringLiteral("A"));
  bind(edited, QStringLiteral("Edited"), QStringLiteral("Z"));
  auto* widget = new DynamicShortcutsWidget();
  widget->populate({&first, &edited});
  QPointer<DynamicShortcutsWidget> watched(widget);
  MsgBox::decision = [](QWidget* parent, const QString&) {
    delete parent;
    return QMessageBox::No;
  };
  catcher(*widget, edited)->setShortcut(QKeySequence(QStringLiteral("A")));
  QVERIFY(watched.isNull());
  QCOMPARE(edited.shortcut(), QKeySequence(QStringLiteral("Z")));
  QCOMPARE(first.shortcut(), QKeySequence(QStringLiteral("A")));
}

void TestShortcutSettings::destroyedAction() {
  auto* action = new QAction();
  bind(*action, QStringLiteral("First"), QStringLiteral("A"));
  DynamicShortcutsWidget widget;
  widget.populate({action});
  auto* binding = catcher(widget, *action);
  delete action;
  QVERIFY(binding->action() == nullptr);
  binding->clearShortcut();
  widget.updateShortcuts();
}

int main(int argc, char** argv) {
  ShortcutSettingsApplication application(argc, argv);
  application.setStyle(QStringLiteral("Fusion"));
  TestShortcutSettings test;
  return QTest::qExec(&test, argc, argv);
}

#include "test_shortcutsettings.moc"
