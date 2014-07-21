#ifndef RENDER_OUTPUT_HPP
#define RENDER_OUTPUT_HPP

#include <dake/gl/gl.hpp>

#include <cmath>
#include <QGLWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>
#include <dake/gl/vertex_array.hpp>

#include "asf.hpp"


class RenderOutput:
    public QGLWidget
{
    Q_OBJECT

    public:
        RenderOutput(QGLFormat fmt, QWidget *parent = nullptr);
        ~RenderOutput(void);

        const dake::math::mat4 &projection(void) const
        { return proj; }
        dake::math::mat4 &projection(void)
        { return proj; }

        const dake::math::mat4 &modelview(void) const
        { return mv; }
        dake::math::mat4 &modelview(void)
        { reload_uniforms = true; return mv; }

        const ASF *asf(void) const
        { return asf_model; }
        ASF *&asf(void)
        { return asf_model; }

        void invalidate(void);

    protected:
        void initializeGL(void);
        void resizeGL(int w, int h);
        void paintGL(void);
        void mousePressEvent(QMouseEvent *evt);
        void mouseReleaseEvent(QMouseEvent *evt);
        void mouseMoveEvent(QMouseEvent *evt);
        void wheelEvent(QWheelEvent *evt);

    private:
        void render_asf(void);
        void render_asf_bone(int bone, const dake::math::mat4 &parent_mv, int depth);

        QTimer *redraw_timer;
        dake::math::mat4 proj, mv;
        dake::math::vec3 light_dir;
        dake::gl::program *bone_prg, *cone_prg, *limit_prg;
        dake::gl::vertex_array *bone_va, *cone_va, *limit_va;
        bool rotate_camera = false, move_camera = false;
        bool reload_uniforms = true;
        float rot_l_x, rot_l_y;
        float fov = static_cast<float>(M_PI) / 4.f;
        int w, h;
        ASF *asf_model = nullptr;
};

#endif
