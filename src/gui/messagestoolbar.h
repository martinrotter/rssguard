#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include "gui/basetoolbar.h"

#include "core/messagesmodel.h"


class MessagesSearchLineEdit;
class QWidgetAction;
class QToolButton;
class QMenu;

class MessagesToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesToolBar(const QString &title, QWidget *parent = 0);
    virtual ~MessagesToolBar();

    // External access to search line edit.
    inline MessagesSearchLineEdit *searchLineEdit() {
      return m_txtSearchMessages;
    }

    // Implementation of BaseToolBar interface.
    QHash<QString, QAction*> availableActions() const;
    QList<QAction*> changeableActions() const;
    void saveChangeableActions(const QStringList &actions);
    void loadChangeableActions();

    // Loads actions as specified by external actions list.
    // NOTE: This is used primarily for reloading actions
    // when they are changed from settings.
    void loadChangeableActions(const QStringList &actions);

  signals:
    // TODO: sem pridat este mozna mode: wildcard, regexp, fixed text.
    // na tuto udalost se navaze filtrovani
    void messageSearchPatternChanged(const QString &pattern);

    // Emitted if message filter is changed.
    void messageFilterChanged(MessagesModel::DisplayFilter filter);

  private slots:
    void handleMessageFilterChange(QAction *action);

  private:
    QWidgetAction *m_actionFilterMessages;
    QToolButton *m_btnFilterMessages;
    QMenu *m_menuFilterMessages;

    QWidgetAction *m_actionSearchMessages;
    MessagesSearchLineEdit *m_txtSearchMessages;
};

#endif // NEWSTOOLBAR_H
