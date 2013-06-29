#ifndef KEYSEQUENCECATCHER_H
#define KEYSEQUENCECATCHER_H

#include <QWidget>
#include <QPushButton>


class QHBoxLayout;
class QToolButton;
class ShortcutButton;

class ShortcutCatcher : public QWidget {
    friend class ShortcutButton;

    Q_OBJECT
  public:
    explicit ShortcutCatcher(QWidget *parent = 0);
    virtual ~ShortcutCatcher();

    void controlModifierlessTimout();
    void updateDisplayShortcut();

  protected slots:
    void startRecording();
    void doneRecording();

  public slots:
    QKeySequence keySequence() const;
    void setKeySequence(const QKeySequence& key);
    void clearKeySequence();

  private:
    QToolButton *m_clearButton;
    ShortcutButton *m_sequenceButton;
    QHBoxLayout *m_layout;

    QKeySequence m_currentSequence;
    QKeySequence m_defaultSequence;

    bool m_isRecording;
    int m_numKey;
    uint m_modifierKeys;
};

#endif // KEYSEQUENCECATCHER_H
