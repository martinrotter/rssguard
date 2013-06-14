#ifndef THEMEFACTORY_H
#define THEMEFACTORY_H

#include <QString>


class ThemeFactory {
  private:
    ThemeFactory();

  public:
    // Adds custom application path to be search for icons.
    static void setupSearchPaths();

    // Returns name of icon theme, which is selected as active for
    // system.
    static QString getSystemIconTheme();

    // Returns list of installed themes, this includes:
    //  a) system-wide themes,
    //  b) application-wide themes.
    static QStringList getInstalledIconThemes();

    // Loads name of selected icon theme for the application and activates it.
    static void loadCurrentIconTheme();

    // Returns name of currently activated theme for the application.
    static QString getCurrentIconTheme();

    // Sets icon theme with given name as the active one.
    static void setCurrentIconTheme(const QString &theme_name);
};

#endif // THEMEFACTORY_H
