#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>


class QProgressBar;
class QToolButton;
class QLabel;

class StatusBar : public QStatusBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit StatusBar(QWidget *parent = 0);
    virtual ~StatusBar();

    QToolButton *fullscreenSwitcher() const;

  private:
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;
    QToolButton *m_fullscreenSwitcher;
    
};

#endif // STATUSBAR_H
