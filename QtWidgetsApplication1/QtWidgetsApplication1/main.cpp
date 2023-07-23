#include "mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowFlags(w.windowFlags() & Qt::MSWindowsFixedSizeDialogHint);
    w.show();
    w.run();

    return a.exec();
}
