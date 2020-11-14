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
    explicit MessagesToolBar(const QString& title, QWidget* parent = nullptr);

    // External access to search line edit.
    inline MessagesSearchLineEdit* searchLineEdit();

    // Implementation of BaseToolBar interface.
    QList<QAction*> availableActions() const;

    QList<QAction*> activatedActions() const;
    void saveAndSetActions(const QStringList& actions);

    // Loads actions as specified by external actions list.
    // NOTE: This is used primarily for reloading actions
    // when they are changed from settings.
    void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false);

    QList<QAction*> convertActions(const QStringList& actions);

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

inline MessagesSearchLineEdit* MessagesToolBar::searchLineEdit() {
  return m_txtSearchMessages;
}

#endif // NEWSTOOLBAR_H
