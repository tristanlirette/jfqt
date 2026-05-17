#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStandardPaths>
#include "jft_process.h"
#include "main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("jfqt");
    app.setApplicationVersion("0.1");

    QCommandLineParser cli;
    cli.setApplicationDescription("Qt6 GUI wrapper for jftui");
    cli.addHelpOption();
    cli.addVersionOption();
    QCommandLineOption pathOpt(
        QStringList{"p", "jftui-path"},
        "Path to jftui executable (default: search PATH)",
        "path");
    cli.addOption(pathOpt);
    cli.process(app);

    QString jftuiPath = cli.isSet(pathOpt)
        ? cli.value(pathOpt)
        : QStandardPaths::findExecutable("jftui");

    if (jftuiPath.isEmpty()) {
        QMessageBox::critical(
            nullptr, "jfqt — Error",
            "Could not find jftui in PATH.\n\n"
            "Install jftui first, or pass --jftui-path <path>.");
        return 1;
    }

    JftProcess process;
    MainWindow window(&process);
    window.show();
    process.start(jftuiPath);

    return app.exec();
}
