#include <QApplication>
#include <QTranslator>
#include <QSystemTrayIcon>
#include <QDir>
#include <QMainWindow>

// Needed for using of ini file format on Mac OS X.
#ifdef Q_OS_MAC
#include <QSettings>
#endif


int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QMainWindow window;
  window.show();

  return QApplication::exec();
}
