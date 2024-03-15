// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef HELPSPOILER_H
#define HELPSPOILER_H

#include <QWidget>

class QTextBrowser;
class QGridLayout;
class QToolButton;
class QParallelAnimationGroup;
class QScrollArea;
class PlainToolButton;

class RSSGUARD_DLLSPEC HelpSpoiler : public QWidget {
    Q_OBJECT

  public:
    explicit HelpSpoiler(QWidget* parent = nullptr);

    void setHelpText(const QString& title, const QString& text, bool is_warning, bool force_html = false);
    void setHelpText(const QString& text, bool is_warning, bool force_html = false);

  private slots:
    void onAnchorClicked(const QUrl& url);

  private:
    QToolButton* m_btnToggle;
    QScrollArea* m_content;
    QParallelAnimationGroup* m_animation;
    QGridLayout* m_layout;
    QTextBrowser* m_text;
    PlainToolButton* m_btnHelp;
};

#endif // HELPSPOILER_H
