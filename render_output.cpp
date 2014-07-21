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
#include <dake/gl/obj.hpp>
#include <dake/gl/shader.hpp>
#include <dake/gl/vertex_array.hpp>
#include <dake/gl/vertex_attrib.hpp>

#include "asf.hpp"
#include "render_output.hpp"


using namespace dake;
using namespace dake::math;


RenderOutput::RenderOutput(QGLFormat fmt, QWidget *pw):
    QGLWidget(fmt, pw),
    proj(mat4::identity()),
    mv  (mat4::identity().translated(vec3(0.f, 0.f, -40.f)))
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
    cone_prg = new gl::program {gl::shader::vert("assets/cone_vert.glsl"), gl::shader::frag("assets/bone_frag.glsl")};

    for (gl::program *prg: {bone_prg, cone_prg}) {
        prg->bind_attrib("in_pos", 0);
        prg->bind_attrib("in_nrm", 1);
        prg->bind_frag("out_col", 0);
    }

    limit_prg = new gl::program {gl::shader::vert("assets/limit_vert.glsl"), gl::shader::geom("assets/limit_geom.glsl"), gl::shader::frag("assets/limit_frag.glsl")};
    limit_prg->bind_frag("out_col", 0);

    // lel
    bone_va = gl::load_obj("assets/cylinder.obj").sections.front().make_vertex_array(0, -1, 1);
    cone_va = gl::load_obj("assets/cone.obj").sections.front().make_vertex_array(0, -1, 1);

    float nothing = 0.f;

    limit_va = new gl::vertex_array;
    limit_va->set_elements(1); // I feel bad
    limit_va->attrib(0)->format(1); // Really bad
    limit_va->attrib(0)->data(&nothing); // (;_;)

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

    for (gl::program *prg: {bone_prg, cone_prg, limit_prg}) {
        prg->use();
        prg->uniform<mat4>("proj") = proj;
    }

    mv.translate(asf_model->root_position);
    render_asf_bone(asf_model->root, mv, 0);
}


static const vec3 colors[] = {
    vec3(.6f, .7f, 1.f),
    vec3(1.f, .25f, 0.f),
    vec3(.2f, 1.f, 0.f),
    vec3(0.f, .25f, 1.f),
    vec3(1.f, 1.f, 0.f),
    vec3(1.f, 1.f, 1.f),
    vec3(1.f, 0.f, .8f),
    vec3(1.f, 0.f, 0.f)
};


void RenderOutput::render_asf_bone(int bi, const mat4 &parent_mv, int hdepth)
{
    ASF::Bone &bone = asf_model->bones[bi];

    mat4 next_mv(parent_mv.translated(bone.length * bone.direction));
    for (int child = bone.first_child; child >= 0; child = asf_model->bones[child].next_sibling) {
        render_asf_bone(child, next_mv, hdepth + 1);
    }

    if (!bone.id) {
        // root
        return;
    }


    // Transformation for (0; 1; 0) -> bone.direction
    vec3 rot_axis;
    float angle;

    if (bone.direction != vec3(0.f, 1.f, 0.f)) {
        rot_axis = vec3(0.f, 1.f, 0.f).cross(bone.direction);
        angle = acosf(bone.direction.y()); // acosf(vec3(0.f, 1.f, 0.f).dot(bone.direction))
    } else {
        rot_axis = vec3(0.f, 0.f, 1.f);
        angle = 0.f;
    }

    mat4 cmv(parent_mv.rotated(angle, rot_axis));


    // Transformation to local coordinate system (as specified by bone.axis)
    mat4 omv(parent_mv.rotated(bone.axis.z(), vec3(0.f, 0.f, 1.f))
                      .rotated(bone.axis.y(), vec3(0.f, 1.f, 0.f))
                      .rotated(bone.axis.x(), vec3(1.f, 0.f, 0.f)));


    for (bool tip: {false, true}) {
        gl::program *prg = tip ? cone_prg : bone_prg;
        gl::vertex_array *va = tip ? cone_va : bone_va;

        prg->use();
        prg->uniform<vec3>("color") = colors[hdepth % 8];
        prg->uniform<float>("length") = bone.length;
        prg->uniform<mat4>("mv") = cmv;
        prg->uniform<mat3>("nrm_mat") = mat3(cmv).transposed_inverse();

        va->draw(GL_TRIANGLES);
    }

    limit_prg->use();
    for (std::pair<const int, std::pair<float, float>> &dof: bone.dof) {
        ASF::Axis axis = static_cast<ASF::Axis>(dof.first);
        std::pair<float, float> &limits = dof.second;

        if ((axis != ASF::RX) && (axis != ASF::RY) && (axis != ASF::RZ)) {
            continue;
        }

        limit_prg->uniform<vec3>("axis") = axis == ASF::RX ? vec3(1.f, 0.f, 0.f)
                                         : axis == ASF::RY ? vec3(0.f, 1.f, 0.f)
                                         :                   vec3(0.f, 0.f, 1.f);
#if 1
        limit_prg->uniform<mat4>("mv") = omv;
#else
        limit_prg->uniform<mat4>("mv") = cmv;
#endif

        float a = limits.first, b = limits.second;
        while (b < 0) {
            b += 2 * static_cast<float>(M_PI);
        }
        while (a > 0) {
            a -= 2 * static_cast<float>(M_PI);
        }
        limit_prg->uniform<float>("l1") = a;
        limit_prg->uniform<float>("l2") = b;

        limit_va->draw(GL_POINTS);
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
        mv = mat4::identity().translated(vec3(-dx / 20.f, dy / 20.f, 0.f)) * mv;
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
