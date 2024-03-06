// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBENGINEPAGE_H
#define WEBENGINEPAGE_H

#include <QWebEnginePage>

class WebEngineViewer;

class WebEnginePage : public QWebEnginePage {
    Q_OBJECT

  public:
    explicit WebEnginePage(QObject* parent = nullptr);

    WebEngineViewer* view() const;

  signals:
    void domIsIdle();

  public slots:
    QString pageHtml(const QString& url);

  private slots:
    void hideUnwantedElements();

  protected:
    virtual bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool is_main_frame);
    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                          const QString& message,
                                          int line_number,
                                          const QString& source_id);
};

#endif // WEBENGINEPAGE_H
