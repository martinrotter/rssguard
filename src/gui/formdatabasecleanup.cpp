#include "gui/formdatabasecleanup.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"


FormDatabaseCleanup::FormDatabaseCleanup(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormDatabaseCleanup) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme("cleanup-database"));


  // TODO: Vytvořil jsem základ okna pro mazání.
  // v tomto okně se nastaví parametry mazání - jak staré zprávy, zda přečtené, zda vakuovat db
  // do třídy DatabaseCleaner se dodělají metody, které budou samotné mazání provádět
  // ve FeedMessageViewer se udělá instance cleaneru stejně jako FeedDownloader (vlákno už tam mam
  // deklarovano (m_dbCleanerThread). Po "OK" tady v tomdle dialogu se pošle signal
  // do instance cleaneru ve feedmessagevieweru.
  // před otevřením tohodle dialogu se zamkne hlavní lock, po oddělání dialogu se odemkne.
  // databasecleaner by mohl reportovat i progress, poběží ve svém vlákně.
}

FormDatabaseCleanup::~FormDatabaseCleanup() {
  delete m_ui;
}
