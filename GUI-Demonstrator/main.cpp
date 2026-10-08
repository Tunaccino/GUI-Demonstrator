#include "controller.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Controller dialog;
    dialog.show();

    return app.exec();
}
