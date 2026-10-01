// For license of this file, see <project-root-folder>/LICENSE.md.

#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "dynamic-shortcuts/dynamicshortcutswidget.h"
#include "dynamic-shortcuts/shortcutcatcher.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"

#include <QAction>
#include <QDir>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QSignalSpy>
#include <QTemporaryDir>
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
    void savedAssignmentsOverrideDefaults();
    void missingAssignmentsUseDefaults();
    void savedLayoutSurvivesDefaultChanges();

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

void TestShortcutSettings::savedAssignmentsOverrideDefaults() {
  QTemporaryDir directory(QDir::current().filePath(QStringLiteral("shortcut-settings-XXXXXX")));
  QVERIFY(directory.isValid());
  Settings settings(directory.filePath(QStringLiteral("shortcuts.ini")), QSettings::IniFormat);
  QAction custom, cleared, sequence;
  custom.setObjectName(QStringLiteral("m_actionUpdateAllItems"));
  cleared.setObjectName(QStringLiteral("m_actionBrowserScrollDown"));
  sequence.setObjectName(QStringLiteral("articlelist_show_unread"));
  custom.setShortcut(QKeySequence(QStringLiteral("Shift+F5")));
  cleared.setShortcut(QKeySequence(QStringLiteral("PgDown")));
  sequence.setShortcut(QKeySequence(QStringLiteral("Ctrl+K, S, U")));
  settings.setValue(GROUP(Keyboard), custom.objectName(), QStringLiteral("Ctrl+Shift+U"));
  settings.setValue(GROUP(Keyboard), cleared.objectName(), QString());
  settings.setValue(GROUP(Keyboard), sequence.objectName(), QStringLiteral("Ctrl+J, U"));
  settings.sync();
  const QStringList keys = settings.allKeys();

  qApp->setSettings(&settings);
  DynamicShortcuts::load({&custom, &cleared, &sequence});
  qApp->setSettings(nullptr);
  QCOMPARE(custom.shortcut(), QKeySequence(QStringLiteral("Ctrl+Shift+U")));
  QVERIFY(cleared.shortcut().isEmpty());
  QCOMPARE(sequence.shortcut(), QKeySequence(QStringLiteral("Ctrl+J, U")));
  QCOMPARE(settings.allKeys(), keys); // Loading does not rewrite the user's profile.
  QCOMPARE(settings.value(GROUP(Keyboard), custom.objectName()).toString(), QStringLiteral("Ctrl+Shift+U"));
  QCOMPARE(settings.value(GROUP(Keyboard), cleared.objectName()).toString(), QString());
  QCOMPARE(settings.value(GROUP(Keyboard), sequence.objectName()).toString(), QStringLiteral("Ctrl+J, U"));
}

void TestShortcutSettings::missingAssignmentsUseDefaults() {
  QTemporaryDir directory(QDir::current().filePath(QStringLiteral("shortcut-settings-XXXXXX")));
  QVERIFY(directory.isValid());
  Settings settings(directory.filePath(QStringLiteral("shortcuts.ini")), QSettings::IniFormat);
  QAction custom, fresh, unbound;
  custom.setObjectName(QStringLiteral("m_actionSettings"));
  fresh.setObjectName(QStringLiteral("m_actionTabsNewBrowser"));
  unbound.setObjectName(QStringLiteral("m_actionClearAllItems"));
  custom.setShortcut(QKeySequence(QStringLiteral("Ctrl+,")));
  fresh.setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
  settings.setValue(GROUP(Keyboard), custom.objectName(), QStringLiteral("Ctrl+S"));

  qApp->setSettings(&settings);
  DynamicShortcuts::load({&custom, &fresh, &unbound});
  qApp->setSettings(nullptr);
  QCOMPARE(custom.shortcut(), QKeySequence(QStringLiteral("Ctrl+S")));
  QCOMPARE(fresh.shortcut(), QKeySequence(QStringLiteral("Ctrl+T")));
  QVERIFY(unbound.shortcut().isEmpty());
  QCOMPARE(settings.allKeys().size(), 1);
}

void TestShortcutSettings::savedLayoutSurvivesDefaultChanges() {
  QTemporaryDir directory(QDir::current().filePath(QStringLiteral("shortcut-settings-XXXXXX")));
  QVERIFY(directory.isValid());
  const QString path = directory.filePath(QStringLiteral("shortcuts.ini"));
  QAction custom, cleared, sequence, legacy;
  custom.setObjectName(QStringLiteral("m_actionUpdateAllItems"));
  cleared.setObjectName(QStringLiteral("m_actionBrowserScrollDown"));
  sequence.setObjectName(QStringLiteral("articlelist_show_unread"));
  legacy.setObjectName(QStringLiteral("m_actionClearAllItems"));
  custom.setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+U")));
  sequence.setShortcut(QKeySequence(QStringLiteral("Ctrl+J, U")));
  legacy.setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
  const QList<QAction*> actions = {&custom, &cleared, &sequence, &legacy};

  {
    Settings settings(path, QSettings::IniFormat);
    qApp->setSettings(&settings);
    DynamicShortcuts::save(actions);
    qApp->setSettings(nullptr);
    settings.sync();
    QCOMPARE(settings.status(), QSettings::NoError);
  }
  custom.setShortcut(QKeySequence(QStringLiteral("Shift+F5")));
  cleared.setShortcut(QKeySequence(QStringLiteral("PgDown")));
  sequence.setShortcut(QKeySequence(QStringLiteral("Ctrl+K, S, U")));
  legacy.setShortcut({});
  Settings reloaded(path, QSettings::IniFormat);
  qApp->setSettings(&reloaded);
  DynamicShortcuts::load(actions);
  qApp->setSettings(nullptr);
  QCOMPARE(custom.shortcut(), QKeySequence(QStringLiteral("Ctrl+Shift+U")));
  QVERIFY(cleared.shortcut().isEmpty());
  QCOMPARE(sequence.shortcut(), QKeySequence(QStringLiteral("Ctrl+J, U")));
  QCOMPARE(legacy.shortcut(), QKeySequence(QStringLiteral("Ctrl+Shift+C")));
  QCOMPARE(reloaded.allKeys().size(), actions.size());
}

int main(int argc, char** argv) {
  ShortcutSettingsApplication application(argc, argv);
  application.setStyle(QStringLiteral("Fusion"));
  TestShortcutSettings test;
  return QTest::qExec(&test, argc, argv);
}

#include "test_shortcutsettings.moc"
