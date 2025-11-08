// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMMAIN_H
#define FORMMAIN_H

#include "ui_formmain.h"

#include <QMainWindow>

class StatusBar;
class QWidgetAction;

class RSSGUARD_DLLSPEC FormMain : public QMainWindow {
    Q_OBJECT

    friend class TabWidget;
    friend class WebBrowser;
    friend class MessagesView;
    friend class FeedsView;

  public:
    explicit FormMain(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowType::Widget);
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

    // Hides all GUI elements so that only tab widget contents are visible.
    void hideShowObtrusiveGuiElements(bool show);

  public slots:
    void display();
    void switchVisibility(bool force_hide = false);
    void switchFullscreenMode();
    void showAddAccountDialog();

  private slots:
    void updateAddItemMenu();
    void updateRecycleBinMenu();
    void updateAccountsMenu();

    void updateTabsButtonsAvailability(int index);
    void updateMessageButtonsAvailability();
    void updateFeedButtonsAvailability();

    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(const Feed* feed, int current, int total);
    void onFeedUpdatesFinished(const FeedDownloadResults& results);

    // Displays various dialogs.
    void backupDatabaseSettings();
    void restoreDatabaseSettings();
    void showDocs();
    void showDbCleanupAssistant();
    void reportABug();
    void donate();

  protected:
    virtual void resizeEvent(QResizeEvent* event);
    virtual void changeEvent(QEvent* event);
    virtual void closeEvent(QCloseEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);

  signals:
    void windowResized(const QSize& new_size);

  private:
    void prepareMenus();
    void createConnections();
    void setupIcons();

    QScopedPointer<Ui::FormMain> m_ui;
    QMenu* m_trayMenu;
    QWidgetAction* m_actionToolbarMainMenu;
    StatusBar* m_statusBar;
};

#endif // FORMMAIN_H
