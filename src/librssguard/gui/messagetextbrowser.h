// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGETEXTBROWSER_H
#define MESSAGETEXTBROWSER_H

#include <QTextBrowser>

#include "core/message.h"

class MessageTextBrowser : public QTextBrowser {
  Q_OBJECT

  public:
    explicit MessageTextBrowser(QWidget* parent = nullptr);
    virtual ~MessageTextBrowser() = default;

    virtual QVariant loadResource(int type, const QUrl& name);
    virtual QSize sizeHint() const;

  protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void resizeEvent (QResizeEvent* event);

  private:
    QPixmap m_imagePlaceholder;
};

#endif // MESSAGETEXTBROWSER_H
