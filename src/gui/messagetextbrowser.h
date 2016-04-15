#ifndef MESSAGETEXTBROWSER_H
#define MESSAGETEXTBROWSER_H

#include <QTextBrowser>



class MessageTextBrowser : public QTextBrowser {
    Q_OBJECT

  public:
    explicit MessageTextBrowser(QWidget *parent = 0);
    virtual ~MessageTextBrowser();

    QVariant loadResource(int type, const QUrl &name);

  signals:
    void imageRequested(const QString &image_url);

  private:
    QPixmap m_imagePlaceholder;
};

#endif // MESSAGETEXTBROWSER_H
