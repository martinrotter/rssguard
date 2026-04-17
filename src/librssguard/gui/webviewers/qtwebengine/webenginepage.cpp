// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtwebengine/webenginepage.h"

#include "definitions/definitions.h"
#include "gui/webviewers/qtwebengine/webengineviewer.h"
#include "miscellaneous/application.h"
#include "network-web/webfactory.h"

#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QWebEngineScript>

WebEnginePage::WebEnginePage(QObject* parent) : QWebEnginePage(qApp->web()->webEngineProfile(), parent) {
  setBackgroundColor(Qt::GlobalColor::transparent);

  connect(this, &WebEnginePage::pdfPrintingFinished, this, &WebEnginePage::onPdfPrintingFinished);
}

WebEngineViewer* WebEnginePage::view() const {
#if QT_VERSION_MAJOR == 6
  return qobject_cast<WebEngineViewer*>(QWebEngineView::forPage(this));
#else
  return qobject_cast<WebEngineViewer*>(QWebEnginePage::view());
#endif
}

void WebEnginePage::onPdfPrintingFinished(const QString& file_path, bool success) {
  if (success) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("PDF printing done"),
                                    tr("PDF printing to file '%1' has finished.").arg(file_path)),
                         GuiMessageDestination(true, true, false),
                         GuiAction(tr("Open destination folder"), this, [file_path]() {
                           qApp->system()->openFolderFile(file_path);
                         }));
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("PDF printing failed"),
                                    tr("PDF printing to file '%1' failed.").arg(file_path),
                                    QSystemTrayIcon::MessageIcon::Critical),
                         GuiMessageDestination(true, true, false));
  }
}

void WebEnginePage::javaScriptAlert(const QUrl& security_origin, const QString& msg) {
  qApp
    ->showGuiMessage(Notification::Event::GeneralEvent,
                     GuiMessage(tr("Website alert"),
                                tr("URL %1 reports this important message: %2").arg(security_origin.toString(), msg)));
}

bool WebEnginePage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool is_main_frame) {
  if (type == NavigationType::NavigationTypeLinkClicked) {
    QTimer::singleShot(500, this, [=]() {
      emit linkMouseClicked(url);
    });

    return false;
  }

  return QWebEnginePage::acceptNavigationRequest(url, type, is_main_frame);
}

void WebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                             const QString& message,
                                             int line_number,
                                             const QString& source_id) {
  Q_UNUSED(level)

  qWarningNN << LOGSEC_JS << message << QSL(" (source: %1:%2)").arg(source_id, QString::number(line_number));
}

QStringList WebEnginePage::chooseFiles(FileSelectionMode mode,
                                       const QStringList& old_files,
                                       const QStringList& accepted_mime_types) {
  return QWebEnginePage::chooseFiles(mode, old_files, accepted_mime_types);
}