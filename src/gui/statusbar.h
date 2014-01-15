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

    inline QToolButton *fullscreenSwitcher() const {
      return m_fullscreenSwitcher;
    }

    // Progress bar operations
    void showProgress(int progress, const QString &label);
    void clearProgress();

  private:
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;
    QToolButton *m_fullscreenSwitcher;
    
};

#endif // STATUSBAR_H
