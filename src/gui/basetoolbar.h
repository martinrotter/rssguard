// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>

class BaseBar {
  public:

    // Returns all actions which can be added to the toolbar.
    virtual QList<QAction*> availableActions() const = 0;

    // Returns all changeable actions which are currently included
    // in the toolbar.
    virtual QList<QAction*> changeableActions() const = 0;

    // Sets new "actions" to the toolbar and perhaps saves the toolbar
    // state into the settings.
    virtual void saveChangeableActions(const QStringList& actions) = 0;

    // Returns list of default actions.
    virtual QStringList defaultActions() const = 0;
    virtual QStringList savedActions() const = 0;

    // Loads the toolbar state from settings.
    virtual void loadSavedActions();
    virtual QList<QAction*> getSpecificActions(const QStringList& actions) = 0;
    virtual void loadSpecificActions(const QList<QAction*>& actions) = 0;

  protected:
    QAction* findMatchingAction(const QString& action, const QList<QAction*>& actions) const;
};

class BaseToolBar : public QToolBar, public BaseBar {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit BaseToolBar(const QString& title, QWidget* parent = 0);
    virtual ~BaseToolBar();
};

#endif // TOOLBAR_H
