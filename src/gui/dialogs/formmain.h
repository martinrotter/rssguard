// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QMainWindow>

#include "ui_formmain.h"

class StatusBar;

class FormMain : public QMainWindow {
  Q_OBJECT

  friend class TabWidget;
  friend class MessagesView;
  friend class FeedsView;

  public:

    // Constructors and destructors.
    explicit FormMain(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~FormMain();

    // Returns menu for the tray icon.
    QMenu* trayMenu() const;

    // Returns global tab widget.
    TabWidget* tabWidget() const;

    // Access to statusbar.
    StatusBar* statusBar() const;

    // Returns list of all globally available actions.
    // NOTE: This is used for setting dynamic shortcuts
    // for given actions.
    QList<QAction*> allActions() const;

    // Loads/saves visual state of the application.
    void loadSize();
    void saveSize();

  public slots:

    // Displays window on top or switches its visibility.
    void display();

    // Switches visibility of main window.
    void switchVisibility(bool force_hide = false);

    // Turns on/off fullscreen mode
    void switchFullscreenMode();

    void showAddAccountDialog();

  private slots:
    void updateAddItemMenu();
    void updateRecycleBinMenu();
    void updateAccountsMenu();

    void updateMessageButtonsAvailability();
    void updateFeedButtonsAvailability();

    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(const Feed* feed, int current, int total);
    void onFeedUpdatesFinished(const FeedDownloadResults& results);

    // Displays various dialogs.
    void backupDatabaseSettings();
    void restoreDatabaseSettings();
    void showWiki();
    void showDbCleanupAssistant();
    void reportABug();
    void donate();

  private:

    // Event handler reimplementations.
    void changeEvent(QEvent* event);

    // Creates all needed menus and sets them up.
    void prepareMenus();

    // Creates needed connections for this window.
    void createConnections();

    // Sets up proper icons for this widget.
    void setupIcons();

    QScopedPointer<Ui::FormMain> m_ui;
    QMenu* m_trayMenu;
    StatusBar* m_statusBar;
};

#endif // FORMMAIN_H
