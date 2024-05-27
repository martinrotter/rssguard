// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/pluginfactory.h"

#include "definitions/definitions.h"
#include "services/abstract/serviceentrypoint.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QPluginLoader>

PluginFactory::PluginFactory() {}

QList<ServiceEntryPoint*> PluginFactory::loadPlugins() const {
  QList<ServiceEntryPoint*> plugins;

  const QString plugin_name_wildcard = pluginNameWildCard();
  const auto plugins_paths = pluginPaths();
  const auto backup_current_dir = QDir::currentPath();

  for (const QString& plugin_folder : plugins_paths) {
    QDirIterator dir_iter(plugin_folder,
                          {plugin_name_wildcard},
                          QDir::Filter::Files,
#if !defined(NDEBUG)
                          QDirIterator::IteratorFlag::Subdirectories);
#else
                          QDirIterator::IteratorFlag::NoIteratorFlags);
#endif

    qDebugNN << LOGSEC_CORE << "Checking for plugins in" << QUOTE_W_SPACE_DOT(plugin_folder);

    while (dir_iter.hasNext()) {
      dir_iter.next();

      const QFileInfo& plugin_file = dir_iter.fileInfo();

      qApp->addLibraryPath(plugin_file.absolutePath());
      QDir::setCurrent(plugin_file.absolutePath());

      QPluginLoader loader(plugin_file.absoluteFilePath());
      ServiceEntryPoint* plugin_instance = qobject_cast<ServiceEntryPoint*>(loader.instance());

      QDir::setCurrent(backup_current_dir);

      if (plugin_instance == nullptr) {
        qCriticalNN << LOGSEC_CORE << "The plugin" << QUOTE_W_SPACE(plugin_file.absoluteFilePath())
                    << "was not loaded successfully:" << QUOTE_W_SPACE_DOT(loader.errorString());
      }
      else {
        qDebugNN << LOGSEC_CORE << "Plugin" << QUOTE_W_SPACE(plugin_file.absoluteFilePath()) << "loaded.";

        plugin_instance->setIsDynamicallyLoaded(true);
        plugins.append(plugin_instance);
      }
    }
  }

  QDir::setCurrent(backup_current_dir);

  return plugins;
}

QStringList PluginFactory::pluginPaths() const {
  QStringList paths;
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  paths << QCoreApplication::applicationDirPath() + QDir::separator() + QL1S("..") + QDir::separator() +
             QL1S(RSSGUARD_LIBDIR) + QDir::separator() + QL1S(APP_LOW_NAME);
#elif defined(Q_OS_WIN)
  paths << QCoreApplication::applicationDirPath() + QDir::separator() + QL1S("plugins");
#else
  paths << QCoreApplication::applicationDirPath();
#endif

#if !defined(NDEBUG)
  paths << QCoreApplication::applicationDirPath() + QDir::separator() + QL1S("..") + QDir::separator();
#endif

  return paths;
}

QString PluginFactory::pluginNameWildCard() const {
  return QSL("*rssguard-*.*");
}
