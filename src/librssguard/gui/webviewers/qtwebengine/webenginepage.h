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
    QList<QAction*> allPageActions() const;

  signals:
    void linkMouseClicked(const QUrl& url);

  private slots:
    void onPdfPrintingFinished(const QString& file_path, bool success);

  protected:
    virtual void javaScriptAlert(const QUrl& security_origin, const QString& msg);
    virtual bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool is_main_frame);
    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                          const QString& message,
                                          int line_number,
                                          const QString& source_id);
    virtual QStringList chooseFiles(FileSelectionMode mode,
                                    const QStringList& old_files,
                                    const QStringList& accepted_mime_types);
};

#endif // WEBENGINEPAGE_H
