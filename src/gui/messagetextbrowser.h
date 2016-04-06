#ifndef MESSAGETEXTBROWSER_H
#define MESSAGETEXTBROWSER_H

#include <QTextBrowser>



class MessageTextBrowser : public QTextBrowser {
  public:
    explicit MessageTextBrowser(QWidget *parent = 0);
    virtual ~MessageTextBrowser();

    QVariant loadResource(int type, const QUrl &name);

  private:
    QPixmap m_imagePlaceholder;
};

#endif // MESSAGETEXTBROWSER_H
