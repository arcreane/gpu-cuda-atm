#include "QtWidgetsApplication2.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QtWidgetsApplication2 window;
    window.show();
    return app.exec();
}
