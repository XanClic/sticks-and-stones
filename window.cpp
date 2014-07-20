#include <dake/gl/gl.hpp>

#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <QApplication>
#include <QGLWidget>
#include <QBoxLayout>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <QComboBox>
#include <QProgressBar>
#include <QScrollArea>

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

    l1 = new QHBoxLayout;
    l1->addWidget(gl);

    i_hate_qt->setLayout(l1);
}

Window::~Window(void)
{
    delete l1;
    delete gl;
    delete i_hate_qt;
}
