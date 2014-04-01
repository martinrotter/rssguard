#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include <QToolBar>

class MessagesToolBar : public QToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesToolBar(const QString &title, QWidget *parent = 0);
    virtual ~MessagesToolBar();

  signals:

  public slots:

};

#endif // NEWSTOOLBAR_H
