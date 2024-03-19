// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "gui/toolbars/basetoolbar.h"

#include <QStatusBar>

class ProgressBarWithText;
class QLabel;
class PlainToolButton;

class StatusBar : public QStatusBar, public BaseBar {
    Q_OBJECT

  public:
    explicit StatusBar(QWidget* parent = nullptr);
    virtual ~StatusBar();

    virtual QList<QAction*> availableActions() const;
    virtual QList<QAction*> activatedActions() const;
    virtual void saveAndSetActions(const QStringList& actions);
    virtual QStringList defaultActions() const;
    virtual QStringList savedActions() const;
    virtual QList<QAction*> convertActions(const QStringList& actions);
    virtual void loadSpecificActions(const QList<QAction*>& actions, bool initial_load = false);

  public slots:
    void showProgressFeeds(int progress, const QString& label);
    void clearProgressFeeds();

    void showProgressDownload(int progress, const QString& tooltip);
    void clearProgressDownload();

  protected:
    bool eventFilter(QObject* watched, QEvent* event);

  private:
    void clear();

    ProgressBarWithText* m_barProgressFeeds;
    QAction* m_barProgressFeedsAction;
    ProgressBarWithText* m_barProgressDownload;
    QAction* m_barProgressDownloadAction;
};

#endif // STATUSBAR_H
