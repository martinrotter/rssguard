#ifndef SYSTEMFACTORY_H
#define SYSTEMFACTORY_H


class SystemFactory {
  private:
    explicit SystemFactory();

  public:
    // Specifies possible states of auto-start functionality.
    enum AutoStartStatus {
      Enabled,
      Disabled,
      Unavailable
    };

    // Returns current status of auto-start function.
    static SystemFactory::AutoStartStatus getAutoStartStatus();

    // Sets new status for auto-start function.
    // Function returns false if setting of
    // new status failed.
    static bool setAutoStartStatus(const SystemFactory::AutoStartStatus &new_status);

#if defined(Q_OS_LINUX)
    // Returns standard location where auto-start .desktop files
    // should be placed.
    static QString getAutostartDesktopFileLocation();
#endif
};

#endif // SYSTEMFACTORY_H
