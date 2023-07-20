#include "mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w("localhost", "8888");
    w.show();
    w.run();

    a.exec();
    return 0;
}
