// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERENTRYPOINT_H
#define INOREADERENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class InoreaderEntryPoint : public ServiceEntryPoint {
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

#endif // INOREADERENTRYPOINT_H
