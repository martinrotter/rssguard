#ifndef THEMEFACTORY_H
#define THEMEFACTORY_H

#include <QString>
#include <QIcon>
#include <QPointer>


class IconThemeFactory : public QObject {
    Q_OBJECT

  public:
    // Singleton getter.
    static IconThemeFactory *getInstance();

    // Destructor.
    virtual ~IconThemeFactory();

    // Wrapper for QIcon::fromTheme.
    // TODO: Na některých platformách ikony bugujou, takže kompletně přepsat.
    // Budu mít vlastní témata, který se budou jednoduše skládat ze
    // složky "mini-kfaenza", ve které budou přímo (bez podsložek)
    // png soubory "application-exit" a další.
    //
    // Tato funkce v zadaném adresáři APP_THEME_PATH/m_currentIconTheme
    // najde danou ikonu a přidá ji do nějakého slovníku (QMap, QHash).
    // Všechny další požadavky na tutéž ikonu se budou brát ze slovníku.
    // Nutnost omezit ikony na nutné minimum (30 dejme tomu).
    QIcon fromTheme(const QString &name);

    // Adds custom application path to be search for icons.
    void setupSearchPaths();

    // Returns list of installed themes, including "default" theme.
    // NOTE: "Default" theme is system theme on Linux and "no theme" on windows.
    QStringList getInstalledIconThemes();

    // Loads name of selected icon theme (from settings) for the application and
    // activates it. If that particular theme is not installed, then
    // "default" theme is loaded.
    // NOTE: All existing widgets get a chance to repaint its icons if
    // notify_widgets is true.
    void loadCurrentIconTheme();

    // Returns name of currently activated theme for the application.
    QString getCurrentIconTheme();

    // Sets icon theme with given name as the active one and loads it.
    void setCurrentIconTheme(const QString &theme_name);

  private:
    QHash<QString, QIcon> m_cachedIcons;

    // Constructor.
    explicit IconThemeFactory(QObject *parent = 0);

    // Holds name of the current icon theme.
    QString m_currentIconTheme;

    // Singleton.
    static QPointer<IconThemeFactory> s_instance;
};

#endif // THEMEFACTORY_H
