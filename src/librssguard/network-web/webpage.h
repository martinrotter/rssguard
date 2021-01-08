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

  private slots:
    void hideUnwantedElements();

  protected:
    virtual bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool is_main_frame);
    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message,
                                          int line_number, const QString& source_id);
};

#endif // WEBPAGE_H
