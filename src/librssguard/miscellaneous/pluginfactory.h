// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef PLUGINFACTORY_H
#define PLUGINFACTORY_H

#include <QStringList>

class ServiceEntryPoint;

class PluginFactory {
  public:
    explicit PluginFactory();

    QList<ServiceEntryPoint*> loadPlugins() const;

  private:
    QStringList pluginPaths() const;
    QString pluginNameSuffix() const;
    QString pluginNameWildCard() const;
};

#endif // PLUGINFACTORY_H
