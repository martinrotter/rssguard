// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSSERVICEENTRYPOINT_H
#define TTRSSSERVICEENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class TtRssServiceEntryPoint : public ServiceEntryPoint {
  public:
    virtual QString name() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
    virtual QString code() const;
    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
};

#endif // TTRSSSERVICEENTRYPOINT_H
