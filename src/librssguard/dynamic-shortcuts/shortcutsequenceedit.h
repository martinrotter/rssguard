// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SHORTCUTSEQUENCEEDIT_H
#define SHORTCUTSEQUENCEEDIT_H

#include <QKeyEvent>
#include <QKeySequenceEdit>

class ShortcutSequenceEdit : public QKeySequenceEdit {
  public:
    explicit ShortcutSequenceEdit(QWidget* parent = nullptr) : QKeySequenceEdit(parent) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
      setFinishingKeyCombinations({});
#endif
    }

  protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    bool event(QEvent* event) override {
      if (event->type() == QEvent::KeyPress) {
        auto* key_event = static_cast<QKeyEvent*>(event);

        if (key_event->key() == Qt::Key_Tab || key_event->key() == Qt::Key_Backtab) {
          // Bypass QWidget's focus navigation, keeping Qt's recorder and timer.
          QKeySequenceEdit::keyPressEvent(key_event);
          return true;
        }
      }

      return QKeySequenceEdit::event(event);
    }
#endif
};

#endif // SHORTCUTSEQUENCEEDIT_H
