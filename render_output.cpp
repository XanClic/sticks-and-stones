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
    reset_transform = true;
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

    proj = mat4::projection(fov, static_cast<float>(w) / h, 1.f, 1000.f);
}


void RenderOutput::paintGL(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (asf_model) {
        if (amc_ani) {
            if (play_animation) {
                if (++frame() >= amc_ani->first_frame() + static_cast<int>(amc_ani->frames().size())) {
                    frame() = amc_ani->first_frame();
                }
            }
        }

        render_asf();
    }

    swapBuffers();
}


void RenderOutput::render_asf(void)
{
    for (gl::program *prg: {bone_prg, cone_prg, limit_prg}) {
        prg->use();
        prg->uniform<mat4>("proj") = proj;
    }

    if (reset_transform) {
        if (amc_ani) {
            if (cur_frame < amc_ani->first_frame()) {
                cur_frame = amc_ani->first_frame();
            } else if (cur_frame >= amc_ani->first_frame() + static_cast<int>(amc_ani->frames().size())) {
                cur_frame = amc_ani->first_frame() + amc_ani->frames().size() - 1;
            }

            amc_ani->apply_frame(cur_frame);
        } else {
            asf_model->reset_transforms();
        }

        reset_transform = false;
    }

    render_asf_bone(asf_model->root_index(), 0);
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


void RenderOutput::render_asf_bone(int bi, int hdepth)
{
    const ASF::Bone &bone = asf_model->bones()[bi];


    if (bone.id) {
        // not root

        mat4 bone_mv(mv * bone.motion_trans * bone.bone_dir_trans);
        mat3 bone_nrm(mat3(bone_mv).transposed_inverse());
        for (bool tip: {false, true}) {
            gl::program *prg = tip ? cone_prg : bone_prg;
            gl::vertex_array *va = tip ? cone_va : bone_va;

            prg->use();
            prg->uniform<vec3>("color") = colors[hdepth % 8];
            prg->uniform<float>("length") = bone.length;
            prg->uniform<mat4>("mv") = bone_mv;
            prg->uniform<mat3>("nrm_mat") = bone_nrm;

            va->draw(GL_TRIANGLES);
        }

        if (limits) {
            limit_prg->use();
            limit_prg->uniform<mat4>("mv") = mv * bone.motion_trans * bone.local_trans;

            for (const std::pair<const int, std::pair<float, float>> &dof: bone.dof) {
                ASF::Axis axis = static_cast<ASF::Axis>(dof.first);
                const std::pair<float, float> &limit = dof.second;

                if ((axis != ASF::RX) && (axis != ASF::RY) && (axis != ASF::RZ)) {
                    continue;
                }

                limit_prg->uniform<vec3>("axis") = axis == ASF::RX ? vec3(1.f, 0.f, 0.f)
                                                 : axis == ASF::RY ? vec3(0.f, 1.f, 0.f)
                                                 :                   vec3(0.f, 0.f, 1.f);

                float a = limit.first, b = limit.second;
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
    }


    for (int child = bone.first_child; child >= 0; child = asf_model->bones()[child].next_sibling) {
        render_asf_bone(child, hdepth + 1);
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
    mv = mat4::identity().translated(vec3(0.f, 0.f, evt->delta() / 50.f)) * mv;

    reload_uniforms = true;
}
