// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGETEXTBROWSER_H
#define MESSAGETEXTBROWSER_H

#include <QTextBrowser>

#include "core/message.h"

class RootItem;

class MessageTextBrowser : public QTextBrowser {
  Q_OBJECT

  public:
    explicit MessageTextBrowser(QWidget* parent = nullptr);
    virtual ~MessageTextBrowser() = default;

    QVariant loadResource(int type, const QUrl& name);

  public slots:
    void clear();
    void reloadFontSettings();
    void loadMessage(const Message& message, RootItem* root);

  protected:
    void wheelEvent(QWheelEvent* e);

  private:
    QString prepareHtmlForMessage(const Message& message);

  private:
    QPixmap m_imagePlaceholder;
    QStringList m_pictures;
};

#endif // MESSAGETEXTBROWSER_H
