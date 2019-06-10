// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDSERVICEENTRYPOINT_H
#define STANDARDSERVICEENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class StandardServiceEntryPoint : public ServiceEntryPoint {
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

#endif // STANDARDSERVICEENTRYPOINT_H
