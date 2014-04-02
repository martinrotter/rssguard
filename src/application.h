#ifndef APPLICATION_H
#define APPLICATION_H

#include "qtsingleapplication/qtsingleapplication.h"


// TODO: presunout nektery veci sem, settings atp
class Application : public QtSingleApplication {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Application(int &argc, char **argv);
    virtual ~Application();

    // Returns pointer to "GOD" application singleton.
    inline static Application *instance() {
      return static_cast<Application*>(QCoreApplication::instance());
    }
};

#endif // APPLICATION_H
