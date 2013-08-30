#ifndef THEMEFACTORY_H
#define THEMEFACTORY_H

#include <QString>
#include <QEvent>
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
    // TODO: If icon is not found in user-defined icon theme,
    // then it is searched in system-default theme (ThemeFactory::getSystemIconTheme()).
    // BUG: I tried to do that, but QIcon is apparently bugged.
    QIcon fromTheme(const QString &name, const QIcon &fallback = QIcon());

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
    void loadCurrentIconTheme(bool notify_widgets);

    // Returns name of currently activated theme for the application.
    QString getCurrentIconTheme();

    // Sets icon theme with given name as the active one and loads it.
    void setCurrentIconTheme(const QString &theme_name);

  private:
    // Constructor.
    explicit IconThemeFactory(QObject *parent = 0);

    // Holds name of the current icon theme.
    QString m_currentIconTheme;

    // Singleton.
    static QPointer<IconThemeFactory> s_instance;
};

class ThemeFactoryEvent : public QEvent {
  public:
    enum Type {
      IconThemeChange = 2000
    };

    // Constructors.
    explicit ThemeFactoryEvent();
    virtual ~ThemeFactoryEvent();

    static QEvent::Type type();

  private:
    static QEvent::Type m_typeOfEvent;
};

#endif // THEMEFACTORY_H
