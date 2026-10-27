#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("SimpleTextEditor"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));
    MainWindow window;
    window.show();
    return app.exec();
}
