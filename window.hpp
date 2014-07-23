#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QBoxLayout>
#include <QWidget>
#include <QComboBox>
#include <QFrame>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QSlider>

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
        void toggle_playback(bool state);
        void set_fps(int count);
        void changed_frame(int frame);
        void set_frame(int frame);

    private:
        QWidget *i_hate_qt;

        RenderOutput *gl;
        QComboBox *amcs;
        QPushButton *load, *play;
        QCheckBox *show_limits, *adapt_limits;
        QSpinBox *fps, *cur_frame;
        QLabel *fps_label, *max_frame;
        QSlider *frame_slider;

        QFrame *frames[1], *vframes[1];

        QHBoxLayout *l1, *l3, *l4;
        QVBoxLayout *l2;

        bool ignore_set_frame = false;
};

#endif
