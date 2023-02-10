// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSTOOLBAR_H
#define FEEDSTOOLBAR_H

#include "gui/toolbars/basetoolbar.h"

#include "gui/reusable/searchlineedit.h"

class QWidgetAction;

class FeedsToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    enum class SearchFields { SearchTitleOnly = 1, SearchAll = 2 };

    explicit FeedsToolBar(const QString& title, QWidget* parent = nullptr);

    virtual QList<QAction*> availableActions() const;
    virtual QList<QAction*> activatedActions() const;
    virtual void saveAndSetActions(const QStringList& actions);
    virtual QList<QAction*> convertActions(const QStringList& actions);
    virtual void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false);
    virtual QStringList defaultActions() const;
    virtual QStringList savedActions() const;

    SearchLineEdit* searchBox() const;

  signals:
    void searchCriteriaChanged(SearchLineEdit::SearchMode mode,
                               Qt::CaseSensitivity sensitivity,
                               int custom_criteria,
                               const QString& phrase);

  private:
    void initializeSearchBox();

  private:
    SearchLineEdit* m_txtSearchMessages;
    QWidgetAction* m_actionSearchMessages;
};

#endif // FEEDSTOOLBAR_H
