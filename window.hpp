#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QBoxLayout>
#include <QWidget>

#include "render_output.hpp"


class Window:
    public QMainWindow
{
    Q_OBJECT

    public:
        Window(void);
        ~Window(void);

        RenderOutput *renderer(void)
        { return gl; }

    private:
        QWidget *i_hate_qt;

        RenderOutput *gl;
        QHBoxLayout *l1;
};

#endif
