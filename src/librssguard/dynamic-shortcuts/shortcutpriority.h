// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SHORTCUTPRIORITY_H
#define SHORTCUTPRIORITY_H

#include <functional>

#include <QKeySequence>
#include <QList>
#include <QPointer>
#include <QWidget>

class QAction;
class QKeyEvent;

// Cooperates with Qt's shortcut map; it does not track multistroke sequences.
class ShortcutPriority {
  public:
    using ActionProvider = std::function<QList<QAction*>()>;
    enum class Decision {
      PassThrough,
      Consume,
      DeliverToEditor
    };

    explicit ShortcutPriority(QWidget* main_window, ActionProvider actions);

    // Editing overrides must reach the widget, then remain accepted. Consume
    // means notify() must return before dispatching to receiver or its filters.
    Decision intercept(QObject* receiver, QEvent* event) const;

    static QList<QKeySequence> keyCandidates(const QKeyEvent& event);
    static bool sequencesConflict(const QKeySequence& first, const QKeySequence& second);

    // Opt a delegated text editor into ImEnabled queries on its focused widget.
    // Native line/text/combo/spin inputs are recognized without registration.
    static void registerInputMethodEditor(QWidget* editor);

  private:
    bool inMainWindow(QObject* receiver) const;
    bool eligible(const QAction* action) const;
    QList<QPointer<QAction>> liveActions() const;
    static bool isRecorder(const QObject* object);
    static bool isEditingKey(const QKeyEvent& event);
    static bool hasEditableFocus();

    QPointer<QWidget> m_mainWindow;
    ActionProvider m_actions;
};

#endif // SHORTCUTPRIORITY_H
