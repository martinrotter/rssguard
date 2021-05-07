// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>

class BaseBar {
  public:

    // Returns all actions which can be added to the toolbar.
    virtual QList<QAction*> availableActions() const = 0;

    // Returns all actions which are currently included
    // in the toolbar.
    virtual QList<QAction*> activatedActions() const = 0;

    // Sets new "actions" to the toolbar and perhaps saves the toolbar
    // state into the settings.
    virtual void saveAndSetActions(const QStringList& actions) = 0;

    // Returns list of default actions.
    virtual QStringList defaultActions() const = 0;

    // Returns list of saved actions.
    virtual QStringList savedActions() const = 0;

    // Loads the toolbar state from settings.
    virtual void loadSavedActions();

    // Converts action names to actions.
    virtual QList<QAction*> convertActions(const QStringList& actions) = 0;

    // Loads list of actions into the bar.
    virtual void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false) = 0;

  protected:
    QAction* findMatchingAction(const QString& action, const QList<QAction*>& actions) const;
};

class BaseToolBar : public QToolBar, public BaseBar {
  Q_OBJECT

  public:
    explicit BaseToolBar(const QString& title, QWidget* parent = nullptr);
    virtual ~BaseToolBar();
};

#endif // TOOLBAR_H
