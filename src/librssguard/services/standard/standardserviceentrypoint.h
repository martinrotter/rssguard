// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDSERVICEENTRYPOINT_H
#define STANDARDSERVICEENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class StandardServiceEntryPoint : public ServiceEntryPoint {
  public:
    virtual QString name() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
    virtual QString code() const;
    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
};

#endif // STANDARDSERVICEENTRYPOINT_H
