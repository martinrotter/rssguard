// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebEnginePage>

class WebViewer;

class WebPage : public QWebEnginePage {
  Q_OBJECT

  public:
    explicit WebPage(QObject* parent = nullptr);

    WebViewer* view() const;

  protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame);
};

#endif // WEBPAGE_H
