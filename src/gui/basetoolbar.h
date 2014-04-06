#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>


class BaseToolBar : public QToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseToolBar(const QString &title, QWidget *parent = 0);
    virtual ~BaseToolBar();

    // Returns all actions which can be added to the toolbar.
    virtual QHash<QString, QAction*> availableActions() const = 0;

    // Returns all changeable actions which are currently included
    // in the toolbar.
    virtual QList<QAction*> changeableActions() const = 0;

    // Sets new "actions" to the toolbar and perhaps saves the toolbar
    // state into the settings.
    virtual void saveChangeableActions(const QStringList &actions) = 0;

    // Loads the toolbar state from settings.
    virtual void loadChangeableActions() = 0;
};

#endif // TOOLBAR_H
