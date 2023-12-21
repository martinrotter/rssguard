// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LIBMPVWIDGET_H
#define LIBMPVWIDGET_H

#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
#include <QOpenGLWidget>

#include <mpv/render_gl.h>

#define BASE_WIDGET QOpenGLWidget
#else
#include <QWidget>
#define BASE_WIDGET QWidget
#endif

struct mpv_handle;

class LibMpvWidget : public BASE_WIDGET {
    Q_OBJECT

    friend class LibMpvBackend;

  public:
    explicit LibMpvWidget(mpv_handle* mpv_handle, QWidget* parent = nullptr);
    virtual ~LibMpvWidget();

    void bind();

  signals:
    void launchMpvEvents();

  private:
    void destroyHandle();

    mpv_handle* m_mpvHandle;

  private slots:
#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
    void maybeUpdate();

  protected:
    virtual void initializeGL();
    virtual void paintGL();

  private:
    static void onMpvRedraw(void* ctx);

    mpv_render_context* m_mpvGL;
#endif
};

#endif // LIBMPVWIDGET_H
