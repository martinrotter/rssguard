// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef AUTOSAVER_H
#define AUTOSAVER_H

#include <QObject>

#include "definitions/definitions.h"

#include <QElapsedTimer>
#include <QTimer>

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

  public slots:
    void changeOccurred();
    void saveIfNeccessary();

  private:
    QTimer m_timer;
    QElapsedTimer m_firstChange;
    int m_maxWaitMsecs;
    int m_periodicSaveMsecs;
    QString m_savingSlot;
};

#endif // AUTOSAVER_H
