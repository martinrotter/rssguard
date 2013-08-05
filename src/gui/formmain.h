#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QMainWindow>
#include <QUrl>

#include "ui_formmain.h"


class FormMain : public QMainWindow {
    Q_OBJECT
    
  public:
    // Constructors and destructors.
    explicit FormMain(QWidget *parent = 0);
    virtual ~FormMain();

    // Returns menu for the tray icon.
    QMenu *getTrayMenu();

    TabWidget *getTabWidget();

    // Returns list of all globally available actions.
    // NOTE: This is used for setting dynamic shortcuts for given actions.
    QList<QAction*> getActions();

    // Singleton accessor.
    static FormMain *getInstance();

  protected:
    // Creates all needed menus and sets them up.
    void prepareMenus();

    // Initializes "Feeds" tab and related stuff.
    void prepareTabs();

    // Creates needed connections for this window.
    void createConnections();

    // Event handler reimplementations.
    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event);

    // Sets up proper icons for this widget.
    // NOTE: All permanent widgets should implement this
    // kind of method and catch ThemeFactoryEvent::type() in its event handler.
    void setupIcons();

  public slots:
    // Processes incoming message from another RSS Guard instance.
    void processExecutionMessage(const QString &message);

    // Quits the application.
    void quit();

    // Displays window on top or switches its visibility.
    void display();

    // Switches visibility of main window.
    void switchVisibility();

    // Turns on/off fullscreen mode
    void switchFullscreenMode(bool turn_fullscreen_on);

  protected slots:
    // Used for last-minute cleanups.
    void cleanupResources();

    // Displays various dialogs.
    void showSettings();
    void showAbout();
    
  private:
    Ui::FormMain *m_ui;
    QMenu *m_trayMenu;

    static FormMain *s_instance;
};

#endif // FORMMAIN_H
