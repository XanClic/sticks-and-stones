#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <QApplication>

#include "asf.hpp"
#include "window.hpp"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <model.asf>\n", argv[0]);
        return 1;
    }

    Window *wnd = new Window;

    std::ifstream asf_str(argv[1]);
    if (!asf_str.is_open()) {
        fprintf(stderr, "%s: Could not open %s: %s\n", argv[0], argv[1], strerror(errno));
        return 1;
    }

    wnd->renderer()->asf() = new ASF(asf_str);
    asf_str.close();

    wnd->show();

    return app.exec();
}
