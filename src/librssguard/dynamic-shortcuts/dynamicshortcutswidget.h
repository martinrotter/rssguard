// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DYNAMICSHORTCUTSWIDGET_H
#define DYNAMICSHORTCUTSWIDGET_H

#include <QWidget>

class QGridLayout;
class ShortcutCatcher;

class DynamicShortcutsWidget : public QWidget {
    Q_OBJECT

  public:
    explicit DynamicShortcutsWidget(QWidget* parent = nullptr);
    virtual ~DynamicShortcutsWidget();

    // Updates shortcuts of all actions according to changes.
    // NOTE: No access to settings is done here.
    // Shortcuts are fetched from settings when applications starts
    // and stored back to settings when application quits.
    void updateShortcuts();

    // Populates this widget with shortcut widgets for given actions.
    // NOTE: This gets initial shortcut for each action from its properties, NOT from
    // the application settings, so shortcuts from settings need to be
    // assigned to actions before calling this method.
    void populate(QList<QAction*> actions);

  private slots:
    void onShortcutChanged(const QKeySequence& sequence);

  signals:
    void setupChanged();

  private:
    QGridLayout* m_layout;
    QList<ShortcutCatcher*> m_actionBindings;
    QHash<QKeySequence, ShortcutCatcher*> m_assignedShortcuts;
};

#endif // DYNAMICSHORTCUTSOVERVIEW_H
