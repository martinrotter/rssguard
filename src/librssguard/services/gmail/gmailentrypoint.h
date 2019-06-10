// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAILENTRYPOINT_H
#define GMAILENTRYPOINT_H

#include "services/abstract/serviceentrypoint.h"

class GmailEntryPoint : public ServiceEntryPoint {
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

#endif // GMAILENTRYPOINT_H
