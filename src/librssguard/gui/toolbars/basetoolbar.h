// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QWidgetAction>

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

    // Returns list of actions which are created solely by this toolbar.
    // NOTE: These actions are added to global list of actions which can be
    // bound to keyboard shortcuts.
    virtual QList<QAction*> extraActions() const;

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
    enum class SearchFields {
      SearchTitleOnly = 1,
      SearchAll = 2
    };

    explicit BaseToolBar(const QString& title, QWidget* parent = nullptr);
    virtual ~BaseToolBar();

  protected:
    void saveToolButtonSelection(const QString& button_name,
                                 const QString& setting_name,
                                 const QList<QAction*>& actions) const;
    void activateAction(const QString& action_name, QWidgetAction* widget_action);
    void addActionToMenu(QMenu* menu,
                         const QIcon& icon,
                         const QString& title,
                         const QString& tooltip_suffix,
                         const QVariant& value,
                         const QString& object_name);
    void drawNumberOfCriterias(QToolButton* btn, int count);
};

#endif // TOOLBAR_H
