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
  //: Name of language, e.g. English.
  QObject::tr("LANG_NAME");
  //: Abbreviation of language, e.g. en.
  //: Use ISO 639-1 code here!
  QObject::tr("LANG_ABBREV");
  //: Version of your translation, e.g. 1.0.
  QObject::tr("LANG_VERSION");
  //: Name of translator - optional.
  QObject::tr("LANG_AUTHOR");
  //: Email of translator - optional.
  QObject::tr("LANG_EMAIL");

  QApplication a(argc, argv);
  QMainWindow window;
  window.show();

  return QApplication::exec();
}
