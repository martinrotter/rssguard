// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMDATABASECLEANUP_H
#define FORMDATABASECLEANUP_H

#include "database/databasecleaner.h"

#include "ui_formdatabasecleanup.h"

#include <QDialog>

class FormDatabaseCleanup : public QDialog {
    Q_OBJECT

  public:
    explicit FormDatabaseCleanup(QWidget* parent = nullptr);
    virtual ~FormDatabaseCleanup() = default;

  protected:
    virtual void closeEvent(QCloseEvent* event);
    virtual void hideEvent(QHideEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

  private slots:
    void updateDaysSuffix(int number);
    void startPurging();
    void onPurgeStarted();
    void onPurgeProgress(int progress, const QString& description);
    void onPurgeFinished();

  signals:
    void purgeRequested(const CleanerOrders& which_data);

  private:
    void loadDatabaseInfo();

  private:
    QScopedPointer<Ui::FormDatabaseCleanup> m_ui;
    DatabaseCleaner m_cleaner;
};

#endif // FORMDATABASECLEANUP_H
