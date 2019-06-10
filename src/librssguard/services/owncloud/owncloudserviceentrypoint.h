// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OWNCLOUDSERVICEENTRYPOINT_H
#define OWNCLOUDSERVICEENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class OwnCloudServiceEntryPoint : public ServiceEntryPoint {
  public:
    ServiceRoot* createNewRoot() const;

    QList<ServiceRoot*> initializeSubtree() const;
    bool isSingleInstanceService() const;
    QString name() const;
    QString code() const;
    QString description() const;
    QString author() const;
    QIcon icon() const;
};

#endif // OWNCLOUDSERVICEENTRYPOINT_H
