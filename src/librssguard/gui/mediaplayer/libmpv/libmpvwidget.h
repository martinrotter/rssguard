// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LIBMPVWIDGET_H
#define LIBMPVWIDGET_H

#include <QWidget>

struct mpv_handle;

class LibMpvWidget : public QWidget {
    Q_OBJECT

  public:
    explicit LibMpvWidget(mpv_handle* mpv_handle, QWidget* parent = nullptr);

    void bind();

  signals:
    void launchMpvEvents();

  private:
    mpv_handle* m_mpvHandle;
};

#endif // LIBMPVWIDGET_H
