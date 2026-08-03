// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ICONFACTORY_H
#define ICONFACTORY_H

#include "definitions/definitions.h"

#include <functional>

#include <QByteArray>
#include <QChar>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QUuid>

class QAction;
class ApplicationPaths;
class Settings;
class QMenu;

class RSSGUARD_DLLSPEC IconFactory : public QObject {
    Q_OBJECT

  public:
    explicit IconFactory(ApplicationPaths* paths, Settings* settings, QObject* parent = nullptr);
    virtual ~IconFactory();

    // Generates round icon of given color.
    static QIcon fromColor(const QColor& color, QChar letter = {});

    static QUuid iconGuid(const QIcon& icon);

    static QAction* iconSelectionMenu(QMenu* menu,
                                      const QList<QIcon>& icons,
                                      const std::function<void(QIcon)>& handler);

    // Used to store/retrieve QIcons from/to Base64-encoded
    // byte array.
    static QIcon fromByteArray(QByteArray array);
    static QByteArray toByteArray(const QIcon& icon);

    static QImage recolorImage(QImage image, const QColor& color);
    static QPixmap recolorPixmap(const QPixmap& pixmap, const QColor& color);

    QString customColoredIconFolder();
    QString customColoredAppIconPath();
    QString customColoredTrayIconPath();
    QString customColoredTrayIconUnreadPath();
    bool generateCustomColoredIcons(const QColor& background_color);
    bool ensureCustomColoredIcons(const QColor& background_color);

    static QPixmap fromByteArray(const QByteArray& array, const QString& format);
    static QByteArray toByteArray(const QPixmap& pixmap, const QString& format);

    static QColor readableTextColor(const QColor& bg);

    // Returns icon from active theme or invalid icon if
    // "no icon theme" is set.
    QIcon fromTheme(const QString& name, const QString& fallback = {});

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
    QString currentIconTheme() const;

    // Sets icon theme with given name as the active one and loads it.
    void setCurrentIconTheme(const QString& theme_name);

  private:
    ApplicationPaths* m_applicationPaths;
    Settings* m_settings;
};

#endif // ICONFACTORY_H
