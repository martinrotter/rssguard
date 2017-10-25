// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ICONFACTORY_H
#define ICONFACTORY_H

#include <QObject>

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QHash>
#include <QIcon>
#include <QString>

class IconFactory : public QObject {
  Q_OBJECT

  public:

    // Constructor.
    explicit IconFactory(QObject* parent = 0);

    // Destructor.
    virtual ~IconFactory();

    // Used to store/retrieve QIcons from/to Base64-encoded
    // byte array.
    static QIcon fromByteArray(QByteArray array);
    static QByteArray toByteArray(const QIcon& icon);

    QPixmap pixmap(const QString& name);

    // Returns icon from active theme or invalid icon if
    // "no icon theme" is set.
    QIcon fromTheme(const QString& name);

    QPixmap miscPixmap(const QString& name);
    QIcon miscIcon(const QString& name);

    // Adds custom application path to be search for icons.
    void setupSearchPaths();

    // Returns list of installed themes, including "default" theme.
    QStringList installedIconThemes() const;

    // Loads name of selected icon theme (from settings) for the application and
    // activates it. If that particular theme is not installed, then
    // "default" theme is loaded.
    void loadCurrentIconTheme();

    // Returns name of currently activated theme for the application.
    inline QString currentIconTheme() const {
      return QIcon::themeName();
    }

    // Sets icon theme with given name as the active one and loads it.
    void setCurrentIconTheme(const QString& theme_name);
};

#endif // ICONFACTORY_H
