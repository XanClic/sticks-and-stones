#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QBoxLayout>
#include <QWidget>
#include <QComboBox>
#include <QFrame>
#include <QCheckBox>
#include <QPushButton>

#include "asf.hpp"
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

    public slots:
        void load_amc(void);
        void refresh_amc(int);

    private:
        QWidget *i_hate_qt;

        RenderOutput *gl;
        QComboBox *amcs;
        QPushButton *load;
        QCheckBox *show_limits, *adapt_limits;

        QFrame *frames[1];

        QHBoxLayout *l1;
        QVBoxLayout *l2;
};

#endif
