// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSSERVICEENTRYPOINT_H
#define TTRSSSERVICEENTRYPOINT_H

#include <librssguard/services/abstract/serviceentrypoint.h>

class TtRssServiceEntryPoint : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.ttrss" FILE "plugin.json")
    Q_INTERFACES(ServiceEntryPoint)

  public:
    explicit TtRssServiceEntryPoint(QObject* parent = nullptr);
    virtual ~TtRssServiceEntryPoint();

    virtual QString name() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
    virtual QString code() const;
    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
};

#endif // TTRSSSERVICEENTRYPOINT_H
