#include <QApplication>
#include "MainWindow.h"
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
    // Hide console window
    FreeConsole();
#endif
    QApplication app(argc, argv);
    
    MainWindow w;
    w.show();
    
    return app.exec();
}
