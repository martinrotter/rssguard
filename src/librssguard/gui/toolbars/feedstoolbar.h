// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSTOOLBAR_H
#define FEEDSTOOLBAR_H

#include "gui/toolbars/basetoolbar.h"

class BaseLineEdit;
class QWidgetAction;

class FeedsToolBar : public BaseToolBar {
  Q_OBJECT

  public:
    explicit FeedsToolBar(const QString& title, QWidget* parent = nullptr);

    virtual QList<QAction*> availableActions() const;
    virtual QList<QAction*> activatedActions() const;
    virtual void saveAndSetActions(const QStringList& actions);
    virtual QList<QAction*> convertActions(const QStringList& actions);
    virtual void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false);
    virtual QStringList defaultActions() const;
    virtual QStringList savedActions() const;

  signals:
    void feedsFilterPatternChanged(const QString& pattern);

  private:
    void initializeSearchBox();

  private:
    BaseLineEdit* m_txtSearchMessages;
    QWidgetAction* m_actionSearchMessages;
};

#endif // FEEDSTOOLBAR_H
