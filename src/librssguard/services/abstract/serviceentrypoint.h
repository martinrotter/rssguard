// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SERVICE_H
#define SERVICE_H

#include <QDialog>
#include <QIcon>
#include <QString>

class ServiceRoot;
class FeedsModel;

// TOP LEVEL class which provides basic information about the "service"
class ServiceEntryPoint {
  public:
    virtual ~ServiceEntryPoint() = default;

    // Creates new service root item, which is ready to be added
    // into the model. This method can for example display
    // some kind of first-time configuration dialog inside itself
    // before returning the root item.
    // Returns NULL if initialization of new root cannot be done.
    virtual ServiceRoot* createNewRoot() const = 0;

    // Performs initialization of all service accounts created using this entry
    // point from persistent DB.
    // Returns list of root nodes which will be afterwards added
    // to the global feed model.
    virtual QList<ServiceRoot*> initializeSubtree() const = 0;

    // Can this service account be added just once?
    // NOTE: This is true particularly for "standard" service
    // which operates with normal RSS/ATOM feeds.
    virtual bool isSingleInstanceService() const = 0;

    // Human readable service name, for example "TT-RSS".
    virtual QString name() const = 0;

    // Some arbitrary string.
    // NOTE: Keep in sync with ServiceRoot::code().
    virtual QString code() const = 0;

    // Human readable service description, for example "Services which offers TT-RSS integration.".
    virtual QString description() const = 0;

    // Author of the service.
    virtual QString author() const = 0;

    // Icon of the service.
    virtual QIcon icon() const = 0;
};

#endif // SERVICE_H
