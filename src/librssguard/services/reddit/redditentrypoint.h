// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITENTRYPOINT_H
#define REDDITENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class RedditEntryPoint : public ServiceEntryPoint {
  public:
    virtual ServiceRoot* createNewRoot() const;
    virtual QList<ServiceRoot*> initializeSubtree() const;
    virtual QString name() const;
    virtual QString code() const;
    virtual QString description() const;
    virtual QString author() const;
    virtual QIcon icon() const;
};

#endif // REDDITENTRYPOINT_H
