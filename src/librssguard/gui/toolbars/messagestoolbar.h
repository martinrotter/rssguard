// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include "core/messagesmodel.h"
#include "core/messagesproxymodel.h"
#include "gui/reusable/searchlineedit.h"
#include "gui/toolbars/basetoolbar.h"

class QWidgetAction;
class QToolButton;
class QMenu;
class QTimer;

class MessagesToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    enum class SearchFields {
      SearchTitleOnly = 1,
      SearchAll = 2
    };

    explicit MessagesToolBar(const QString& title, QWidget* parent = nullptr);

    virtual QList<QAction*> availableActions() const;
    virtual QList<QAction*> activatedActions() const;
    virtual void saveAndSetActions(const QStringList& actions);
    virtual void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false);
    virtual QList<QAction*> convertActions(const QStringList& actions);
    virtual QStringList defaultActions() const;
    virtual QStringList savedActions() const;

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
    void addActionToMenu(QMenu* menu,
                         const QIcon& icon,
                         const QString& title,
                         const QVariant& value,
                         const QString& name);
    void initializeHighlighter();
    void activateAction(const QString& action_name, QWidgetAction* widget_action);
    void saveToolButtonSelection(const QString& button_name, const QList<QAction*>& actions) const;
    void drawNumberOfCriterias(QToolButton* btn, int count);

  private:
    QWidgetAction* m_actionMessageHighlighter;
    QWidgetAction* m_actionMessageFilter;
    QToolButton* m_btnMessageHighlighter;
    QToolButton* m_btnMessageFilter;
    QMenu* m_menuMessageHighlighter;
    QMenu* m_menuMessageFilter;
    QWidgetAction* m_actionSearchMessages;
    SearchLineEdit* m_txtSearchMessages;
};

#endif // NEWSTOOLBAR_H
