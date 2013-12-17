#ifndef SYSTEMFACTORY_H
#define SYSTEMFACTORY_H

#include <QObject>
#include <QPointer>


class SystemFactory : public QObject {
    Q_OBJECT

  private:
    explicit SystemFactory(QObject *parent = 0);

  public:
    virtual ~SystemFactory();

    // Specifies possible states of auto-start functionality.
    enum AutoStartStatus {
      Enabled,
      Disabled,
      Unavailable
    };

    // Returns current status of auto-start function.
    SystemFactory::AutoStartStatus getAutoStartStatus();

    // Sets new status for auto-start function.
    // Function returns false if setting of
    // new status failed.
    bool setAutoStartStatus(const SystemFactory::AutoStartStatus &new_status);

#if defined(Q_OS_LINUX)
    // Returns standard location where auto-start .desktop files
    // should be placed.
    static QString getAutostartDesktopFileLocation();
#endif

    // Singleton getter.
    static SystemFactory *getInstance();

  private:
    static QPointer<SystemFactory> s_instance;
};

#endif // SYSTEMFACTORY_H
