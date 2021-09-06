// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef HELPSPOILER_H
#define HELPSPOILER_H

#include <QWidget>

class QLabel;
class QGridLayout;
class QToolButton;
class QParallelAnimationGroup;
class QScrollArea;

class HelpSpoiler : public QWidget {
  Q_OBJECT

  public:
    explicit HelpSpoiler(QWidget* parent = nullptr);

    void setHelpText(const QString& text, bool is_warning);

  private:
    QToolButton* m_btnToggle;
    QScrollArea* m_content;
    QParallelAnimationGroup* m_animation;
    QGridLayout* m_layout;
    QLabel* m_text;
};

#endif // HELPSPOILER_H
