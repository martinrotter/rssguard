// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SERVICE_H
#define SERVICE_H

#include <QDialog>
#include <QIcon>
#include <QString>

class ServiceRoot;
class FeedsModel;

// TOP LEVEL class which provides basic information about the "service"
class RSSGUARD_DLLSPEC ServiceEntryPoint {
  public:
    virtual ~ServiceEntryPoint() = default;

    // Creates new service root item, which is ready to be added
    // into the model. This method can for example display
    // some kind of first-time configuration dialog inside itself
    // before returning the root item.
    // Returns nullptr if initialization of new root cannot be done.
    virtual ServiceRoot* createNewRoot() const = 0;

    // Performs initialization of all service accounts created using this entry
    // point from persistent DB.
    // Returns list of root nodes which will be afterwards added
    // to the global feed model.
    virtual QList<ServiceRoot*> initializeSubtree() const = 0;

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

    bool isDynamicallyLoaded() const;
    void setIsDynamicallyLoaded(bool dynamic);

  private:
    bool m_isDynamicallyLoaded = false;
};

inline bool ServiceEntryPoint::isDynamicallyLoaded() const {
  return m_isDynamicallyLoaded;
}

inline void ServiceEntryPoint::setIsDynamicallyLoaded(bool dynamic) {
  m_isDynamicallyLoaded = dynamic;
}

Q_DECLARE_INTERFACE(ServiceEntryPoint, "io.github.martinrotter.rssguard.serviceentrypoint")

#endif // SERVICE_H
