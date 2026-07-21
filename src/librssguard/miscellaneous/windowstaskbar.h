// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WINDOWSTASKBAR_H
#define WINDOWSTASKBAR_H

#include <QtGlobal>

#if defined(Q_OS_WIN)

#include <QIcon>
#include <QImage>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QtGui/qwindowdefs.h>

struct ITaskbarList4;
class QAction;

class WindowsTaskbar : public QObject {
  public:
    explicit WindowsTaskbar(QObject* parent = nullptr);
    ~WindowsTaskbar();

    bool isAvailable() const;

    void setThumbnailActions(const QList<QAction*>& actions, const QIcon& pause_icon, const QIcon& resume_icon);
    void setThumbnailButtonsEnabled(bool enabled);
    void thumbnailButtonsCreated(WId window_id);
    bool triggerThumbnailButton(quint32 button_id);

    bool setUnreadOverlayIcon(WId window_id, int number, bool show_pause) const;
    bool clearOverlayIcon(WId window_id) const;
    bool setProgressValue(WId window_id, qulonglong current, qulonglong total) const;
    bool clearProgress(WId window_id) const;

  private:
    struct ThumbnailButton {
        int m_id;
        QIcon m_icon;
        QString m_tooltip;
        bool m_enabled;
        bool m_visible;
    };

    void updateThumbnailButtons();
    void hideThumbnailButtons();
    QList<ThumbnailButton> thumbnailButtons(bool visible) const;
    QImage generateOverlayIcon(int number, bool show_pause) const;
    bool setOverlayIcon(WId window_id, const QImage& icon) const;
    bool isThumbnailButtonReady() const;
    bool setThumbnailButtons(WId window_id, const QList<ThumbnailButton>& buttons, bool add) const;
    bool reportResult(long result, const QString& operation) const;

  private:
    ITaskbarList4* m_taskbar;
    QList<QPointer<QAction>> m_thumbnailActions;
    QIcon m_pauseIcon;
    QIcon m_resumeIcon;
    WId m_thumbnailWindowId;
    bool m_thumbnailButtonsEnabled;
    bool m_thumbnailButtonsReady;
    bool m_thumbnailButtonsAdded;
};

#endif

#endif // WINDOWSTASKBAR_H
