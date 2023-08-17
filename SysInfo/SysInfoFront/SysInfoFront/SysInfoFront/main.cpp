#include "sysinfofront.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SysInfoFront w;
    w.setFixedSize(w.width(), w.height());
    w.show();
    return a.exec();
}
