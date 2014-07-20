#include <dake/gl/gl.hpp>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <QGLWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>
#include <dake/gl/vertex_array.hpp>
#include <dake/gl/vertex_attrib.hpp>

#include "asf.hpp"
#include "render_output.hpp"


using namespace dake;
using namespace dake::math;
using namespace dake::gl;


RenderOutput::RenderOutput(QGLFormat fmt, QWidget *pw):
    QGLWidget(fmt, pw),
    proj(mat4::identity()),
    mv  (mat4::identity().translated(vec3(0.f, 0.f, -5.f)))
{
    redraw_timer = new QTimer(this);
    connect(redraw_timer, SIGNAL(timeout()), this, SLOT(updateGL()));
}

RenderOutput::~RenderOutput(void)
{
    delete bone_prg;
}


void RenderOutput::invalidate(void)
{
    reload_uniforms = true;
}


void RenderOutput::initializeGL(void)
{
    gl::glext_init();

    int maj, min;
    glGetIntegerv(GL_MAJOR_VERSION, &maj);
    glGetIntegerv(GL_MINOR_VERSION, &min);

    printf("Received OpenGL %i.%i\n", maj, min);

    glViewport(0, 0, width(), height());
    glClearColor(0.f, 0.f, 0.f, 1.f);

    glEnable(GL_DEPTH_TEST);

    bone_prg = new gl::program {gl::shader::vert("assets/bone_vert.glsl"), gl::shader::frag("assets/bone_frag.glsl")};
    bone_prg->bind_attrib("in_pos", 0);
    bone_prg->bind_frag("out_col", 0);

    bone_va = new gl::vertex_array;
    bone_va->set_elements(2);

    float line_vert[] = {
        0.f, 1.f
    };
    bone_va->attrib(0)->format(1);
    bone_va->attrib(0)->data(line_vert);

    redraw_timer->start(0);
}


void RenderOutput::resizeGL(int wdt, int hgt)
{
    w = wdt;
    h = hgt;

    glViewport(0, 0, w, h);

    proj = mat4::projection(fov, static_cast<float>(w) / h, .1f, 100.f);
}


void RenderOutput::paintGL(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (asf_model) {
        render_asf();
    }

    swapBuffers();
}


void RenderOutput::render_asf(void)
{
    mat4 cmv(mv);

    bone_prg->use();
    bone_prg->uniform<mat4>("proj") = proj;

    mv.translate(asf_model->root_position);
    render_asf_bone(asf_model->root, mv, 0);
}


static const vec3 colors[] = {
    vec3(1.f, .25f, 0.f),
    vec3(.2f, 1.f, 0.f),
    vec3(0.f, .25f, 1.f),
    vec3(1.f, 1.f, 0.f),
    vec3(1.f, 1.f, 1.f),
    vec3(1.f, 0.f, .8f),
    vec3(1.f, 0.f, 0.f),
    vec3(.6f, .7f, 1.f)
};


void RenderOutput::render_asf_bone(int bi, const mat4 &parent_mv, int hdepth)
{
    ASF::Bone &bone = asf_model->bones[bi];

    bone_prg->uniform<vec3>("color") = colors[hdepth % 8];
    bone_prg->uniform<vec3>("direction") = bone.direction;
    bone_prg->uniform<float>("length") = bone.length;
    bone_prg->uniform<mat4>("mv") = parent_mv;

    bone_va->draw(GL_LINES);

    for (int child = bone.first_child; child >= 0; child = asf_model->bones[child].next_sibling) {
        mat4 cmv(parent_mv.translated(bone.length * bone.direction));
        render_asf_bone(child, cmv, hdepth + 1);
    }
}


void RenderOutput::mousePressEvent(QMouseEvent *evt)
{
    if ((evt->button() == Qt::LeftButton) && !move_camera) {
        grabMouse(Qt::ClosedHandCursor);

        rotate_camera = true;

        rot_l_x = evt->x();
        rot_l_y = evt->y();
    } else if ((evt->button() == Qt::RightButton) && !rotate_camera) {
        grabMouse(Qt::ClosedHandCursor);

        move_camera = true;

        rot_l_x = evt->x();
        rot_l_y = evt->y();
    }
}


void RenderOutput::mouseReleaseEvent(QMouseEvent *evt)
{
    if ((evt->button() == Qt::LeftButton) && rotate_camera) {
        releaseMouse();
        rotate_camera = false;
    } else if ((evt->button() == Qt::RightButton) && move_camera) {
        releaseMouse();
        move_camera = false;
    }
}


void RenderOutput::mouseMoveEvent(QMouseEvent *evt)
{
    int dx = evt->x() - rot_l_x;
    int dy = evt->y() - rot_l_y;

    if (rotate_camera) {
        mv = mat4::identity().rotated(dy / 4.f * static_cast<float>(M_PI) / 180.f, vec3(1.f, 0.f, 0.f))
                             .rotated(dx / 4.f * static_cast<float>(M_PI) / 180.f, vec3(0.f, 1.f, 0.f))
                            * mv;
    } else {
        mv = mat4::identity().translated(vec3(-dx / 100.f, dy / 100.f, 0.f)) * mv;
    }

    reload_uniforms = true;

    rot_l_x = evt->x();
    rot_l_y = evt->y();
}


void RenderOutput::wheelEvent(QWheelEvent *evt)
{
    mv = mat4::identity().translated(vec3(0.f, 0.f, evt->delta() / 360.f)) * mv;

    reload_uniforms = true;
}