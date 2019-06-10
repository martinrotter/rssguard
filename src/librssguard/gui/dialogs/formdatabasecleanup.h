// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMDATABASECLEANUP_H
#define FORMDATABASECLEANUP_H

#include <QDialog>

#include "ui_formdatabasecleanup.h"

#include "miscellaneous/databasecleaner.h"

class FormDatabaseCleanup : public QDialog {
  Q_OBJECT

  public:
    explicit FormDatabaseCleanup(QWidget* parent = nullptr);
    virtual ~FormDatabaseCleanup() = default;

  protected:
    void closeEvent(QCloseEvent* event);
    void keyPressEvent(QKeyEvent* event);

  private slots:
    void updateDaysSuffix(int number);
    void startPurging();
    void onPurgeStarted();
    void onPurgeProgress(int progress, const QString& description);
    void onPurgeFinished(bool finished);

  signals:
    void purgeRequested(const CleanerOrders& which_data);

  private:
    void loadDatabaseInfo();

  private:
    QScopedPointer<Ui::FormDatabaseCleanup> m_ui;
    DatabaseCleaner m_cleaner;
};

#endif // FORMDATABASECLEANUP_H
