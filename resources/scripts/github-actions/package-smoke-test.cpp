// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/tabwidget.h"
#include "gui/webbrowser.h"
#include "gui/webviewers/qtextbrowser/textbrowserviewer.h"
#include "miscellaneous/application.h"

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include "gui/webviewers/qtwebengine/webenginepage.h"
#include "gui/webviewers/qtwebengine/webengineviewer.h"
#endif

#include <QTimer>
#include <cstdio>
#include <cstdlib>

int rssguardMain(int argc, char* argv[]);

namespace {
  const QString marker = QStringLiteral("RSS Guard packaged article smoke test");
  const char* rendered_variant = nullptr;

  void finishSmokeTest(const QString& text, const char* variant) {
    if (rendered_variant != nullptr || !text.contains(marker)) {
      return; // An earlier blank-page load may finish before our article.
    }

    rendered_variant = variant;
    std::fprintf(stderr, "Packaged %s article rendered; requesting normal application shutdown.\n", variant);
    // Let the WebEngine callback return before Qt closes windows and unwinds modal dialogs.
    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
  }

  void scheduleSmokeTest() {
    QTimer::singleShot(30000, QCoreApplication::instance(), [] {
      std::fprintf(stderr, "Packaged article viewer failed to render within 30 seconds.\n");
      QCoreApplication::exit(EXIT_FAILURE);
    });

    // Called by rssguardMain only after real application startup has finished.
    QTimer::singleShot(0, QCoreApplication::instance(), [] {
      if (qApp->mainForm() == nullptr) {
        QCoreApplication::exit(EXIT_FAILURE);
        return;
      }

      WebBrowser* browser = qApp->mainForm()->tabWidget()->feedMessageViewer()->webBrowser();
      const QString html = QStringLiteral("<!doctype html><html><body><article><h1>%1</h1>"
                                          "<p>A local article verifies the packaged viewer without network access.</p>"
                                          "</article></body></html>").arg(marker);

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
      auto* web_viewer = dynamic_cast<WebEngineViewer*>(browser->viewer());
      if (web_viewer != nullptr) {
        QObject::connect(web_viewer, &QWebEngineView::loadFinished, web_viewer, [web_viewer](bool success) {
          if (success) {
            web_viewer->QWebEngineView::page()->toPlainText([](const QString& text) { finishSmokeTest(text, "web"); });
          }
        });
        browser->setHtml(html);
        return;
      }
#endif

      auto* text_viewer = dynamic_cast<TextBrowserViewer*>(browser->viewer());
      if (text_viewer == nullptr) {
        QCoreApplication::exit(EXIT_FAILURE);
        return;
      }

      browser->setHtml(html);
      QTimer::singleShot(0, text_viewer, [text_viewer] { finishSmokeTest(text_viewer->plainText(), "text"); });
    });
  }
}

void startPackageSmokeTest() {
  scheduleSmokeTest();
}

int main(int argc, char* argv[]) {
  const int result = rssguardMain(argc, argv);

  // rssguardMain destroys the real main window and Application before returning.
  if (result == EXIT_SUCCESS && rendered_variant != nullptr) {
    std::printf("RSSGUARD_PACKAGE_SMOKE_OK:%s\n", rendered_variant);
    std::fflush(stdout);
    return EXIT_SUCCESS;
  }

  std::fprintf(stderr, "Packaged viewer did not finish rendering and shutdown successfully (exit code %d).\n", result);
  return result == EXIT_SUCCESS ? EXIT_FAILURE : result;
}
