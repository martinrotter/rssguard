#ifndef THEMEFACTORY_H
#define THEMEFACTORY_H

#include <QString>
#include <QEvent>
#include <QIcon>


class ThemeFactory {
  private:
    ThemeFactory();

  public:
    // Adds custom application path to be search for icons.
    static void setupSearchPaths();

    // Returns list of installed themes, this includes:
    //  a) system-wide themes,
    //  b) application-wide themes.
    static QStringList getInstalledIconThemes();

    // Loads name of selected icon theme for the application and activates it.
    // NOTE: All existing widgets get a chance to repaint its icons if
    // notify_widgets is true.
    static void loadCurrentIconTheme(bool notify_widgets);

    // Returns name of currently activated theme for the application.
    static QString getCurrentIconTheme();

    // Sets icon theme with given name as the active one.
    static void setCurrentIconTheme(const QString &theme_name);

    // Wrapper for QIcon::fromTheme.
    // TODO: If icon is not found in user-defined icon theme,
    // then it is searched in system-default theme (ThemeFactory::getSystemIconTheme()).
    // BUG: I tried to do that, but QIcon is apparently bugged.
    static QIcon fromTheme(const QString & name, const QIcon & fallback = QIcon());
};

class ThemeFactoryEvent : public QEvent {
  public:
    enum Type {
      IconThemeChange = 2000
    };

    ThemeFactoryEvent();
    virtual ~ThemeFactoryEvent();

    static QEvent::Type type();

  private:
    static QEvent::Type m_typeOfEvent;
};

#endif // THEMEFACTORY_H
