// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>

#include "gui/basetoolbar.h"

class QProgressBar;
class QLabel;
class PlainToolButton;

class StatusBar : public QStatusBar, public BaseBar {
  Q_OBJECT

  public:
    explicit StatusBar(QWidget* parent = nullptr);
    virtual ~StatusBar();

    QList<QAction*> availableActions() const;
    QList<QAction*> activatedActions() const;
    void saveAndSetActions(const QStringList& actions);
    QStringList defaultActions() const;
    QStringList savedActions() const;
    QList<QAction*> convertActions(const QStringList& actions);
    void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false);

  public slots:
    void showProgressFeeds(int progress, const QString& label);
    void clearProgressFeeds();

    void showProgressDownload(int progress, const QString& tooltip);
    void clearProgressDownload();

  protected:
    bool eventFilter(QObject* watched, QEvent* event);

  private:
    void clear();

    QProgressBar* m_barProgressFeeds;
    QAction* m_barProgressFeedsAction;
    QLabel* m_lblProgressFeeds;
    QAction* m_lblProgressFeedsAction;
    QProgressBar* m_barProgressDownload;
    QAction* m_barProgressDownloadAction;
    QLabel* m_lblProgressDownload;
    QAction* m_lblProgressDownloadAction;
};

#endif // STATUSBAR_H
