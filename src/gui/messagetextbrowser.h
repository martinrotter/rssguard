// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGETEXTBROWSER_H
#define MESSAGETEXTBROWSER_H

#include <QTextBrowser>

class MessageTextBrowser : public QTextBrowser {
  Q_OBJECT

  public:
    explicit MessageTextBrowser(QWidget* parent = nullptr);
    virtual ~MessageTextBrowser() = default;

    QVariant loadResource(int type, const QUrl& name);

  protected:
    void wheelEvent(QWheelEvent* e);

  private:
    QPixmap m_imagePlaceholder;
};

#endif // MESSAGETEXTBROWSER_H
