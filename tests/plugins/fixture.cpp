// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/serviceentrypoint.h"

#include <QFile>

class FixturePlugin : public QObject, public ServiceEntryPoint {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.github.martinrotter.rssguard.fixture" FILE TEST_PLUGIN_METADATA)
    Q_INTERFACES(ServiceEntryPoint)

  public:
    FixturePlugin() {
      QFile marker(QString::fromUtf8(qgetenv("RSSGUARD_PLUGIN_CONSTRUCTOR_MARKER")));
      if (marker.open(QIODevice::WriteOnly)) {
        marker.write("constructed");
      }
    }

    ServiceRoot* createNewRoot() const override {
      return nullptr;
    }
    QList<ServiceRoot*> initializeSubtree() const override {
      return {};
    }
    QString name() const override {
      return QStringLiteral("Fixture");
    }
    QString code() const override {
      return QStringLiteral("fixture");
    }
    QString description() const override {
      return {};
    }
    QString author() const override {
      return {};
    }
    QIcon icon() const override {
      return {};
    }
};

#include "fixture.moc"
