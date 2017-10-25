// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSSERVICEENTRYPOINT_H
#define TTRSSSERVICEENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class TtRssServiceEntryPoint : public ServiceEntryPoint {
  public:
    bool isSingleInstanceService() const;
    QString name() const;
    QString description() const;
    QString author() const;
    QIcon icon() const;
    QString code() const;

    ServiceRoot* createNewRoot() const;

    QList<ServiceRoot*> initializeSubtree() const;
};

#endif // TTRSSSERVICEENTRYPOINT_H
