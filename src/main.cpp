#include <QMainWindow>

#include "qtsingleapplication/qtsingleapplication.h"


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

  // TODO: Finish implementation of QtSingleApplication into RSS Guard.
  // This primarily concerns slot in FormMain which reacts when application is launched
  // repeatedly. See 'trivial' example from QtSingleApplication source code for more
  // information.
  QtSingleApplication application(argc, argv);
  QMainWindow window;
  window.show();

  return QtSingleApplication::exec();
}
