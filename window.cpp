#include <dake/gl/gl.hpp>

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <typeinfo>
#include <unistd.h>

#include <QApplication>
#include <QGLWidget>
#include <QBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QStringList>
#include <QMessageBox>
#include <QVariant>
#include <QIcon>
#include <QSpinBox>
#include <QLabel>
#include <QSlider>

#include "amc.hpp"
#include "render_output.hpp"
#include "window.hpp"


static int fls(int mask)
{
    int i = 0;

    while (mask) {
        i++;
        mask >>= 1;
    }

    return i;
}


// Thanks to Qt quality (they are idiots)
static const int ogl_version[] = {
    0x11, 0x12, 0x13, 0x14, 0x15, 0x20, 0x21, 0x21,
    0x21, 0x21, 0x21, 0x21, 0x30, 0x31, 0x32, 0x33,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x44, 0x44, 0x44,
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44
};


Window::Window(void):
    QMainWindow(nullptr)
{
    i_hate_qt = new QWidget(this);
    setCentralWidget(i_hate_qt);


    for (QFrame *&f: frames) {
        f = new QFrame;
        f->setFrameShape(QFrame::HLine);
    }

    for (QFrame *&f: vframes) {
        f = new QFrame;
        f->setFrameShape(QFrame::VLine);
    }


    load = new QPushButton("Load AMC");
    amcs = new QComboBox;
    amcs->addItem("(none)", 0);

    play = new QPushButton(QIcon::fromTheme("media-playback-start"), "");
    play->setCheckable(true);
    fps = new QSpinBox;
    fps->setRange(1, 1000);
    fps->setValue(60);
    fps_label = new QLabel("FPS");

    cur_frame = new QSpinBox;
    cur_frame->setRange(-1, -1);
    cur_frame->setValue(-1);
    max_frame = new QLabel(" / ?");

    frame_slider = new QSlider(Qt::Horizontal);
    frame_slider->setRange(-1, -1);
    frame_slider->setValue(-1);

    play->setEnabled(false);
    fps->setEnabled(false);
    cur_frame->setEnabled(false);
    frame_slider->setEnabled(false);

    show_limits = new QCheckBox("Show limits");
    adapt_limits = new QCheckBox("Adapt to still bones");

    l3 = new QHBoxLayout;
    l3->addWidget(play);
    l3->addWidget(vframes[0]);
    l3->addWidget(fps);
    l3->addWidget(fps_label);

    l4 = new QHBoxLayout;
    l4->addWidget(cur_frame, 1);
    l4->addWidget(max_frame, 1);

    l2 = new QVBoxLayout;
    l2->addWidget(load);
    l2->addWidget(amcs);
    l2->addLayout(l3);
    l2->addLayout(l4);
    l2->addWidget(frame_slider);
    l2->addWidget(frames[0]);
    l2->addWidget(show_limits);
    l2->addWidget(adapt_limits);
    l2->addStretch();


    int highest = fls(QGLFormat::openGLVersionFlags());
    if (!highest) {
        throw std::runtime_error("No OpenGL support");
    }

    int version = ogl_version[highest - 1];

    QGLFormat fmt;
    fmt.setProfile(QGLFormat::CoreProfile);
    fmt.setVersion(version >> 4, version & 0xf);

    if (((version >> 4) < 3) || (((version >> 4) == 3) && ((version & 0xf) < 3))) {
        throw std::runtime_error("OpenGL 3.3+ is required");
    }

    printf("Selecting OpenGL %i.%i Core\n", version >> 4, version & 0xf);

    gl = new RenderOutput(fmt);

    connect(show_limits, SIGNAL(stateChanged(int)), gl, SLOT(show_limits(int)));
    connect(adapt_limits, SIGNAL(stateChanged(int)), gl, SLOT(adapt_limits(int)));

    connect(load, SIGNAL(pressed()), this, SLOT(load_amc()));
    connect(amcs, SIGNAL(currentIndexChanged(int)), this, SLOT(refresh_amc(int)));
    connect(play, SIGNAL(toggled(bool)), this, SLOT(toggle_playback(bool)));
    connect(fps, SIGNAL(valueChanged(int)), this, SLOT(set_fps(int)));
    connect(cur_frame, SIGNAL(valueChanged(int)), this, SLOT(set_frame(int)));
    connect(frame_slider, SIGNAL(valueChanged(int)), this, SLOT(set_frame(int)));

    connect(gl, SIGNAL(frame_changed(int)), this, SLOT(changed_frame(int)));

    l1 = new QHBoxLayout;
    l1->addWidget(gl, 1);
    l1->addLayout(l2);

    i_hate_qt->setLayout(l1);
}


Window::~Window(void)
{
    for (int i = 0; i < amcs->count(); i++) {
        delete reinterpret_cast<AMC *>(static_cast<uintptr_t>(amcs->itemData(i).value<qulonglong>()));
    }

    delete l1;
    delete l2;
    delete l3;
    delete l4;
    delete gl;
    delete adapt_limits;
    delete show_limits;
    delete frame_slider;
    delete max_frame;
    delete cur_frame;
    delete fps_label;
    delete fps;
    delete play;
    delete amcs;
    delete load;

    for (QFrame *f: vframes) {
        delete f;
    }
    for (QFrame *f: frames) {
        delete f;
    }

    delete i_hate_qt;
}


void Window::load_amc(void)
{
    QStringList paths = QFileDialog::getOpenFileNames(this, "Load AMC", QString(), "Animations (*.amc);;All files (*.*)");

    for (const auto &path: paths) {
        std::ifstream inp(path.toUtf8().constData());
        if (!inp.is_open()) {
            QMessageBox::critical(this, "Could not open file", QString("Could not open ") + path + QString(": ") + QString(strerror(errno)));
            continue;
        }

        try {
            std::string name_copy(path.toUtf8().constData());
            QString name(basename(name_copy.c_str()));

            AMC *amc = new AMC(inp, gl->asf());
            // i haet qt
            amcs->addItem(name, static_cast<qulonglong>(reinterpret_cast<uintptr_t>(amc)));
        } catch (std::exception &e) {
            QMessageBox::critical(this, "Error", QString("Could not load ") + path + QString(": ") + QString(e.what()));
        }
    }

    refresh_amc(-1);

    gl->invalidate();
}


void Window::refresh_amc(int)
{
    if (amcs->currentIndex() < 0) {
        gl->amc() = nullptr;
    } else {
        gl->amc() = reinterpret_cast<AMC *>(static_cast<uintptr_t>(amcs->currentData().value<qulonglong>()));
    }

    bool has_amc = gl->amc();
    play->setEnabled(has_amc);
    fps->setEnabled(has_amc);
    cur_frame->setEnabled(has_amc);
    frame_slider->setEnabled(has_amc);

    if (has_amc) {
        int min = gl->amc()->first_frame();
        int max = min + gl->amc()->frames().size();
        cur_frame->setRange(min, max);
        cur_frame->setValue(gl->frame());
        max_frame->setText(QString(" / %1").arg(max));
        frame_slider->setRange(min, max);
        frame_slider->setValue(gl->frame());
    } else {
        cur_frame->setRange(-1, -1);
        cur_frame->setValue(-1);
        max_frame->setText(" / ?");
        frame_slider->setRange(-1, -1);
        frame_slider->setValue(-1);
    }
}


void Window::toggle_playback(bool state)
{
    gl->play(state);
}


void Window::set_fps(int count)
{
    gl->fps() = count;
}


void Window::changed_frame(int frame)
{
    ignore_set_frame = true;

    cur_frame->setValue(frame);
    frame_slider->setValue(frame);

    ignore_set_frame = false;
}


void Window::set_frame(int frame)
{
    if (ignore_set_frame) {
        return;
    }

    gl->frame() = frame;

    cur_frame->setValue(frame);
    frame_slider->setValue(frame);
}
