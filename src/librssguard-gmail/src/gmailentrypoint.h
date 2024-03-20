// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAILENTRYPOINT_H
#define GMAILENTRYPOINT_H

#include <librssguard/services/abstract/serviceentrypoint.h>

class GmailEntryPoint : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.gmail" FILE "plugin.json")
    Q_INTERFACES(ServiceEntryPoint)

  public:
    explicit GmailEntryPoint(QObject* parent = nullptr);
    virtual ~GmailEntryPoint();

    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
    virtual QString name() const;
    virtual QString code() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
};

#endif // GMAILENTRYPOINT_H
