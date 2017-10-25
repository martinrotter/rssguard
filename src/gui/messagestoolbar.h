// For license of this file, see <project-root-folder>/LICENSE.md.

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
    explicit MessagesToolBar(const QString& title, QWidget* parent = 0);
    virtual ~MessagesToolBar();

    // External access to search line edit.
    inline MessagesSearchLineEdit* searchLineEdit() {
      return m_txtSearchMessages;
    }

    // Implementation of BaseToolBar interface.
    QList<QAction*> availableActions() const;

    QList<QAction*> changeableActions() const;
    void saveChangeableActions(const QStringList& actions);

    // Loads actions as specified by external actions list.
    // NOTE: This is used primarily for reloading actions
    // when they are changed from settings.
    void loadSpecificActions(const QList<QAction*>& actions);

    QList<QAction*> getSpecificActions(const QStringList& actions);

    QStringList defaultActions() const;
    QStringList savedActions() const;

  signals:
    void messageSearchPatternChanged(const QString& pattern);

    // Emitted if message filter is changed.
    void messageFilterChanged(MessagesModel::MessageHighlighter filter);

  private slots:

    // Called when highlighter gets changed.
    void handleMessageHighlighterChange(QAction* action);

  private:
    void initializeSearchBox();
    void initializeHighlighter();

  private:
    QWidgetAction* m_actionMessageHighlighter;
    QToolButton* m_btnMessageHighlighter;
    QMenu* m_menuMessageHighlighter;
    QWidgetAction* m_actionSearchMessages;
    MessagesSearchLineEdit* m_txtSearchMessages;
};

#endif // NEWSTOOLBAR_H
