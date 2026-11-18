// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GCALENDARENTRYPOINT_H
#define GCALENDARENTRYPOINT_H

#include <librssguard/services/abstract/serviceentrypoint.h>

class GCalendarEntryPoint : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.gcalendar" FILE "plugin.json")
    Q_INTERFACES(ServiceEntryPoint)

  public:
    explicit GCalendarEntryPoint(QObject* parent = nullptr);
    virtual ServiceRoot* createNewRoot() const override;
    virtual QList<ServiceRoot*> initializeSubtree() const override;
    virtual QString name() const override;
    virtual QString code() const override;
    virtual QString description() const override;
    virtual QString author() const override;
    virtual QIcon icon() const override;
};

#endif // GCALENDARENTRYPOINT_H
