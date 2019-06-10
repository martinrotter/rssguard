// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef AUTOSAVER_H
#define AUTOSAVER_H

#include <QBasicTimer>
#include <QObject>
#include <QTime>

class AutoSaver : public QObject {
  Q_OBJECT

  public:
    explicit AutoSaver(QObject* parent);
    virtual ~AutoSaver();

    void saveIfNeccessary();

  public slots:
    void changeOccurred();

  protected:
    void timerEvent(QTimerEvent* event);

  private:
    QBasicTimer m_timer;
    QTime m_firstChange;
};

#endif // AUTOSAVER_H
