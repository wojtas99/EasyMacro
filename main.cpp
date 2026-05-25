#include <QApplication>
#include "src/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    QApplication::setOrganizationName("EasyMacro");
    QApplication::setApplicationName("EasyMacro");

    MainWindow window;
    window.show();

    return app.exec();
}
