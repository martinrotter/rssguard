// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERENTRYPOINT_H
#define GREADERENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class GreaderEntryPoint : public ServiceEntryPoint {
  public:
    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
    virtual QString name() const;
    virtual QString code() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
};

#endif // GREADERENTRYPOINT_H
