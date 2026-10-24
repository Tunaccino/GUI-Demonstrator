#include "controller.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Die Ports koennen beim Einstecken die Plaetze tauschen. Feste Namen gibt
    // es unter /dev/serial/by-id/, zum Testen ohne Hardware das virtuelle Board.
    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption lowerOption(QStringLiteral("lower"),
                                         QStringLiteral("Serieller Port des unteren Boards (Schrittmotor)."),
                                         QStringLiteral("port"), QStringLiteral("/dev/ttyACM0"));
    const QCommandLineOption upperOption(QStringLiteral("upper"),
                                         QStringLiteral("Serieller Port des oberen Boards (BLDC)."),
                                         QStringLiteral("port"), QStringLiteral("/dev/ttyACM1"));
    parser.addOptions({lowerOption, upperOption});
    parser.process(app);

    Controller dialog(parser.value(lowerOption), parser.value(upperOption));
    dialog.show();

    return app.exec();
}
