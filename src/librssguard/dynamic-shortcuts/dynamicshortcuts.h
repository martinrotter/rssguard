// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DYNAMICSHORTCUTS_H
#define DYNAMICSHORTCUTS_H

#include <QList>

class QAction;

class DynamicShortcuts {
  public:

    // Checks the application settings and then initializes shortcut of
    // each action from actions from the settings.
    static void load(const QList<QAction*>& actions);

    // Stores shortcut of each action from actions into the application
    // settings.
    static void save(const QList<QAction*>& actions);

  private:

    // Constructor.
    explicit DynamicShortcuts() = default;
};

#endif // DYNAMICSHORTCUTS_H
