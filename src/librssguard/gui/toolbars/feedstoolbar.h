// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSTOOLBAR_H
#define FEEDSTOOLBAR_H

#include "core/feedsproxymodel.h"
#include "gui/reusable/searchlineedit.h"
#include "gui/toolbars/basetoolbar.h"

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

    SearchLineEdit* searchBox() const;

  signals:
    void feedFilterChanged(FeedsProxyModel::FeedListFilter filter);
    void searchCriteriaChanged(SearchLineEdit::SearchMode mode,
                               Qt::CaseSensitivity sensitivity,
                               int custom_criteria,
                               const QString& phrase);

  private slots:
    void handleMessageFilterChange(QAction* action);

  private:
    void initializeFilter();
    void initializeSearchBox();

  private:
    QWidgetAction* m_actionMessageFilter;
    QToolButton* m_btnMessageFilter;
    QMenu* m_menuMessageFilter;

    SearchLineEdit* m_txtSearchMessages;
    QWidgetAction* m_actionSearchMessages;
};

#endif // FEEDSTOOLBAR_H
