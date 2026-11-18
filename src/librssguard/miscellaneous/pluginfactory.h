// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef PLUGINFACTORY_H
#define PLUGINFACTORY_H

#include <QList>
#include <QStringList>

class ServiceEntryPoint;

class RSSGUARD_DLLSPEC PluginFactory {
  public:
    explicit PluginFactory();

    QList<ServiceEntryPoint*> loadPlugins() const;
    static ServiceEntryPoint* loadPlugin(const QString& path, QString* error = nullptr);

  private:
    QStringList pluginPaths() const;
    QString pluginNameWildCard() const;
};

#endif // PLUGINFACTORY_H
