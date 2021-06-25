#include <iostream>
#include <fstream>
#include <locale>

#include <QApplication>
#include <QtGui>

#include "main_window.h"



int main(int argc, char **argv)
{

    QApplication app(argc, argv);
    CMainWindow main_w;
    main_w.show();
    std::setlocale(LC_ALL, "C");
    return app.exec();
}
