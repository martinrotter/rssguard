// This work is derived from MRichTextEditor.
//
// For license of this file, see <project-root-folder>/resources/text/COPYING_GNU_LGPL_21.

#ifndef MTEXTEDIT_H_
#define MTEXTEDIT_H_

#include <QImage>
#include <QMimeData>
#include <QTextEdit>

class MTextEdit : public QTextEdit {
    Q_OBJECT

  public:
    MTextEdit(QWidget* parent);

    void dropImage(const QImage& image, const QString& format);

  protected:
    bool canInsertFromMimeData(const QMimeData* source) const;
    void insertFromMimeData(const QMimeData* source);
    QMimeData* createMimeDataFromSelection() const;
};

#endif
