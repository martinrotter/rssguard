// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEXTCLOUDSERVICEENTRYPOINT_H
#define NEXTCLOUDSERVICEENTRYPOINT_H

#include <librssguard/services/abstract/serviceentrypoint.h>

class NextcloudServiceEntryPoint : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.nextcloud" FILE "plugin.json")
    Q_INTERFACES(ServiceEntryPoint)

  public:
    explicit NextcloudServiceEntryPoint(QObject* parent = nullptr);
    virtual ~NextcloudServiceEntryPoint();

    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
    virtual QString name() const;
    virtual QString code() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
};

#endif // NEXTCLOUDSERVICEENTRYPOINT_H
