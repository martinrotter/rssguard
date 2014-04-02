#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include "gui/basetoolbar.h"


class BaseLineEdit;

class MessagesToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesToolBar(const QString &title, QWidget *parent = 0);
    virtual ~MessagesToolBar();

    // Operations with changeable actions.
    QList<QAction*> changeableActions() const;
    void saveChangeableActions() const;
    void loadChangeableActions();

  signals:

  public slots:

  private:
    QWidget *m_spacer;
    BaseLineEdit *m_txtFilter;

};

#endif // NEWSTOOLBAR_H
