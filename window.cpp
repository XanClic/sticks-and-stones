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


    load = new QPushButton("Load AMC");
    amcs = new QComboBox;
    show_limits = new QCheckBox("Show limits");

    l2 = new QVBoxLayout;
    l2->addWidget(load);
    l2->addWidget(amcs);
    l2->addWidget(frames[0]);
    l2->addWidget(show_limits);
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

    connect(load, SIGNAL(pressed()), this, SLOT(load_amc()));
    connect(amcs, SIGNAL(currentIndexChanged(int)), this, SLOT(refresh_amc(int)));

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
    delete gl;
    delete show_limits;
    delete amcs;
    delete load;

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
}
