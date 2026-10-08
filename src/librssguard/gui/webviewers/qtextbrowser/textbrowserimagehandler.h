// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TEXTBROWSERIMAGEHANDLER_H
#define TEXTBROWSERIMAGEHANDLER_H

#include "definitions/definitions.h"

#include <QObject>
#include <QTextFormat>
#include <QTextObjectInterface>

class RSSGUARD_DLLSPEC TextBrowserImageHandler : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

  public:
    static constexpr int MaximumHeightProperty = QTextFormat::UserProperty + 1;

    explicit TextBrowserImageHandler(QTextObjectInterface* native_handler, QObject* parent);

    void setZoomFactor(qreal zoom_factor);

    QSizeF intrinsicSize(QTextDocument* document, int position, const QTextFormat& format) override;
    void drawObject(QPainter* painter,
                    const QRectF& rect,
                    QTextDocument* document,
                    int position,
                    const QTextFormat& format) override;

  private:
    // The native handler and this wrapper are owned by the same document layout.
    QTextObjectInterface* m_nativeHandler;
    qreal m_zoomFactor = 1.0;
};

#endif // TEXTBROWSERIMAGEHANDLER_H
