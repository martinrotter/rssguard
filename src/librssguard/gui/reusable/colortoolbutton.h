// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COLORTOOLBUTTON_H
#define COLORTOOLBUTTON_H

#include <optional>

#include <QNetworkProxy>
#include <QToolButton>

class RSSGUARD_DLLSPEC ColorIconToolButton : public QToolButton {
    Q_OBJECT

  public:
    explicit ColorIconToolButton(QWidget* parent = nullptr);

    void appendExtraAction(QAction* action);

    bool colorOnlyMode() const;
    void setColorOnlyMode(bool color_only_mode);

    QColor color() const;
    void setColor(const QColor& color, bool inform_about_changes = true);

    QColor alternateColor() const;
    void setAlternateColor(const QColor& alt_color);

    QIcon icon() const;
    void setIcon(const QIcon& icon, bool inform_about_changes = true);

    std::optional<QIcon> defaultIcon() const;
    void setDefaultIcon(const QIcon& icon);

    QList<QIcon> additionalIcons() const;
    void setAdditionalIcons(const QList<QIcon>& icons);

    QString suggestedUrl() const;
    void setSuggestedUrl(const QString& url);

    std::optional<QNetworkProxy> proxy() const;
    void setProxy(const QNetworkProxy& proxy);

  public slots:
    void setRandomColor();

  private slots:
    void askForColor();
    void getIconFromUrl();
    void askForIcon();
    void updateMenu();

  signals:
    void colorChanged(const QColor& new_color);
    void iconChanged(const QIcon& icon);

  protected:
    virtual void mouseReleaseEvent(QMouseEvent* event);

  private:
    QColor m_color;
    QColor m_alternateColor;
    bool m_colorOnlyMode;
    std::optional<QIcon> m_defaultIcon;
    QList<QIcon> m_additionalIcons;
    QMenu* m_menu;
    QString m_suggestedUrl;
    std::optional<QNetworkProxy> m_proxy;
    QList<QAction*> m_extraActions;
};

#endif // COLORTOOLBUTTON_H
