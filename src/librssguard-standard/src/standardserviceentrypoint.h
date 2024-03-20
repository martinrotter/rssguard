// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDSERVICEENTRYPOINT_H
#define STANDARDSERVICEENTRYPOINT_H

#include <librssguard/services/abstract/serviceentrypoint.h>

class StandardServiceEntryPoint : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.standard" FILE "plugin.json")
    Q_INTERFACES(ServiceEntryPoint)

  public:
    explicit StandardServiceEntryPoint(QObject* parent = nullptr);
    virtual ~StandardServiceEntryPoint();

    virtual QString name() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
    virtual QString code() const;
    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
};

#endif // STANDARDSERVICEENTRYPOINT_H
