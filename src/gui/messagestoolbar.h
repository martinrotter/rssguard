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

    // Operations with changeable actions.
    QList<QAction*> availableActions() const;
    QList<QAction*> changeableActions() const;
    void saveChangeableActions() const;
    void saveChangeableActions(const QStringList &actions);
    void loadChangeableActions();

  signals:

  public slots:

  private:
    QWidgetAction *m_actionFilter;
    BaseLineEdit *m_txtFilter;
};

#endif // NEWSTOOLBAR_H
