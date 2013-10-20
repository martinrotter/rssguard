#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>


class StatusBar : public QStatusBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit StatusBar(QWidget *parent = 0);
    virtual ~StatusBar();

  signals:
    
  public slots:
    
};

#endif // STATUSBAR_H
