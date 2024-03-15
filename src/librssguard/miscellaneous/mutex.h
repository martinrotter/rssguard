// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MUTEX_H
#define MUTEX_H

#include <QMutex>
#include <QObject>

class RSSGUARD_DLLSPEC Mutex : public QObject {
    Q_OBJECT

  public:
    explicit Mutex(QObject* parent = nullptr);
    virtual ~Mutex();

    // Main methods.
    bool tryLock();
    bool tryLock(int timeout);

    // Identifies if mutes is locked or not.
    bool isLocked() const;

    operator QMutex*() const;

  public slots:
    void lock();
    void unlock();

  protected:
    // These methods set proper value for m_isLocked and emit signals.
    void setLocked();
    void setUnlocked();

  signals:
    void locked();
    void unlocked();

  private:
    QScopedPointer<QMutex> m_mutex;
    bool m_isLocked;
};

#endif // MUTEX_H
