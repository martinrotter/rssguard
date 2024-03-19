// This work is derived from MRichTextEditor.
//
// For license of this file, see <project-root-folder>/resources/text/COPYING_GNU_LGPL_21.

#include "gui/richtexteditor/mtextedit.h"

#include "definitions/definitions.h"

#include <stdlib.h>

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QTextCursor>
#include <QTextDocument>

MTextEdit::MTextEdit(QWidget* parent) : QTextEdit(parent) {}

bool MTextEdit::canInsertFromMimeData(const QMimeData* source) const {
  return source->hasImage() || QTextEdit::canInsertFromMimeData(source);
}

void MTextEdit::insertFromMimeData(const QMimeData* source) {
  if (source->hasImage()) {
    QStringList formats = source->formats();
    QString format;

    for (int i = 0; i < formats.size(); i++) {
      if (formats[i] == QSL("image/bmp")) {
        format = QSL("BMP");
        break;
      }
      if (formats[i] == QSL("image/jpeg")) {
        format = QSL("JPG");
        break;
      }
      if (formats[i] == QSL("image/jpg")) {
        format = QSL("JPG");
        break;
      }
      if (formats[i] == QSL("image/gif")) {
        format = QSL("GIF");
        break;
      }
      if (formats[i] == QSL("image/png")) {
        format = QSL("PNG");
        break;
      }
      if (formats[i] == QSL("image/pbm")) {
        format = QSL("PBM");
        break;
      }
      if (formats[i] == QSL("image/pgm")) {
        format = QSL("PGM");
        break;
      }
      if (formats[i] == QSL("image/ppm")) {
        format = QSL("PPM");
        break;
      }
      if (formats[i] == QSL("image/tiff")) {
        format = QSL("TIFF");
        break;
      }
      if (formats[i] == QSL("image/xbm")) {
        format = QSL("XBM");
        break;
      }
      if (formats[i] == QSL("image/xpm")) {
        format = QSL("XPM");
        break;
      }
    }
    if (!format.isEmpty()) {
      dropImage(qvariant_cast<QImage>(source->imageData()), QSL("JPG"));
      return;
    }
  }

  QTextEdit::insertFromMimeData(source);
}

QMimeData* MTextEdit::createMimeDataFromSelection() const {
  return QTextEdit::createMimeDataFromSelection();
}

void MTextEdit::dropImage(const QImage& image, const QString& format) {
  QByteArray bytes;
  QBuffer buffer(&bytes);

  buffer.open(QIODevice::OpenModeFlag::WriteOnly);
  image.save(&buffer, format.toLocal8Bit().data());
  buffer.close();

  QByteArray base64 = bytes.toBase64();
  QByteArray base64l;

  for (int i = 0; i < base64.size(); i++) {
    base64l.append(base64[i]);

    if (i % 80 == 0) {
      base64l.append("\n");
    }
  }

  QTextCursor cursor = textCursor();
  QTextImageFormat image_format;

  image_format.setWidth(image.width());
  image_format.setHeight(image.height());
  image_format.setName(QSL("data:image/%1;base64,%2").arg(QSL("%1.%2").arg(rand()).arg(format)).arg(base64l.data()));
  cursor.insertImage(image_format);
}
