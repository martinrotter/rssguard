// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "gui/reusable/searchlineedit.h"
#include "gui/toolbars/basetoolbar.h"

class QMenu;
class QTimer;

class MessagesToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    explicit MessagesToolBar(const QString& title, QWidget* parent = nullptr);

    virtual QList<QAction*> availableActions() const;
    virtual QList<QAction*> activatedActions() const;
    virtual void saveAndSetActions(const QStringList& actions);
    virtual void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false);
    virtual QList<QAction*> convertActions(const QStringList& actions);
    virtual QStringList defaultActions() const;
    virtual QStringList savedActions() const;
    virtual QList<QAction*> extraActions() const;

    SearchLineEdit* searchBox() const;

  signals:
    void searchCriteriaChanged(SearchLineEdit::SearchMode mode,
                               Qt::CaseSensitivity sensitivity,
                               int custom_criteria,
                               const QString& phrase);
    void messageHighlighterChanged(MessagesModel::MessageHighlighter highlighter);
    void messageFilterChanged(MessagesProxyModel::MessageListFilter filter);

  private slots:
    void handleMessageHighlighterChange(QAction* action);
    void handleMessageFilterChange(QAction* action);

  private:
    void initializeSearchBox();
    void initializeHighlighter();

  private:
    QWidgetAction* m_actionMessageHighlighter;
    QToolButton* m_btnMessageHighlighter;
    QMenu* m_menuMessageHighlighter;

    QWidgetAction* m_actionMessageFilter;
    QToolButton* m_btnMessageFilter;
    QMenu* m_menuMessageFilter;

    QWidgetAction* m_actionSearchMessages;
    SearchLineEdit* m_txtSearchMessages;
};

#endif // NEWSTOOLBAR_H
