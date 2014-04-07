#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include "gui/basetoolbar.h"


class BaseLineEdit;
class QWidgetAction;

class MessagesToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesToolBar(const QString &title, QWidget *parent = 0);
    virtual ~MessagesToolBar();

    inline BaseLineEdit *searchLineEdit() {
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

  public slots:

  private:
    QWidgetAction *m_actionSearchMessages;
    BaseLineEdit *m_txtSearchMessages;
};

#endif // NEWSTOOLBAR_H
