// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPENTRYPOINT_H
#define XMPPENTRYPOINT_H

#include <librssguard/services/abstract/serviceentrypoint.h>

class XmppEntryPoint : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.xmpp" FILE "plugin.json")
    Q_INTERFACES(ServiceEntryPoint)

  public:
    explicit XmppEntryPoint(QObject* parent = nullptr);
    virtual ~XmppEntryPoint();

    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
    virtual QString name() const;
    virtual QString code() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
};

#endif // XMPPENTRYPOINT_H
