#ifndef THEMEFACTORY_H
#define THEMEFACTORY_H

#include "core/defs.h"

#include <QString>
#include <QIcon>
#include <QPointer>
#include <QHash>
#include <QDir>


class IconThemeFactory : public QObject {
    Q_OBJECT

  public:
    // Singleton getter.
    static IconThemeFactory *instance();

    // Destructor.
    virtual ~IconThemeFactory();

    // Returns icon from active theme or invalid icon if
    // "no icon theme" is set.
    inline QIcon fromTheme(const QString &name) {
      if (m_currentIconTheme == APP_NO_THEME) {
        return QIcon();
      }

      if (!m_cachedIcons.contains(name)) {
        // Icon is not cached yet.
        m_cachedIcons.insert(name, QIcon(APP_THEME_PATH + QDir::separator() +
                                         m_currentIconTheme + QDir::separator() +
                                         name + APP_THEME_SUFFIX));
      }

      return m_cachedIcons.value(name);
    }

    // Adds custom application path to be search for icons.
    void setupSearchPaths();

    // Returns list of installed themes, including "default" theme.
    QStringList installedIconThemes();

    // Loads name of selected icon theme (from settings) for the application and
    // activates it. If that particular theme is not installed, then
    // "default" theme is loaded.
    void loadCurrentIconTheme();

    // Returns name of currently activated theme for the application.
    QString currentIconTheme();

    // Sets icon theme with given name as the active one and loads it.
    void setCurrentIconTheme(const QString &theme_name);

  private:
    // Constructor.
    explicit IconThemeFactory(QObject *parent = 0);

    QHash<QString, QIcon> m_cachedIcons;
    QString m_currentIconTheme;

    // Singleton.
    static QPointer<IconThemeFactory> s_instance;
};

#endif // THEMEFACTORY_H
