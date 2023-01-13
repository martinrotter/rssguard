// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef AUTOSAVER_H
#define AUTOSAVER_H

#include <QObject>

#include "definitions/definitions.h"

#include <QBasicTimer>
#include <QElapsedTimer>

#define AUTOSAVE_IN (1000 * 3) // In seconds.
#define MAX_WAIT (1000 * 15)   // In seconds.

class AutoSaver : public QObject {
    Q_OBJECT

  public:
    explicit AutoSaver(QObject* parent,
                       const QString& saving_slot = QSL("save"),
                       int max_wait_secs = MAX_WAIT,
                       int periodic_save_secs = AUTOSAVE_IN);
    virtual ~AutoSaver();

    void saveIfNeccessary();

  public slots:
    void changeOccurred();

  protected:
    void timerEvent(QTimerEvent* event);

  private:
    QBasicTimer m_timer;
    QElapsedTimer m_firstChange;
    int m_maxWaitSecs;
    int m_periodicSaveSecs;
    QString m_savingSlot;
};

#endif // AUTOSAVER_H
