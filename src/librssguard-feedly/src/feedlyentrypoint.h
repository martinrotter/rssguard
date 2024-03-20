// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDLYENTRYPOINT_H
#define FEEDLYENTRYPOINT_H

#include <librssguard/services/abstract/serviceentrypoint.h>

class FeedlyEntryPoint : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.feedly" FILE "plugin.json")
    Q_INTERFACES(ServiceEntryPoint)

  public:
    explicit FeedlyEntryPoint(QObject* parent = nullptr);
    virtual ~FeedlyEntryPoint();

    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
    virtual QString name() const;
    virtual QString code() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
};

#endif // FEEDLYENTRYPOINT_H
