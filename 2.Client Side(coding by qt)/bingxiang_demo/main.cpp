#include "mainwindow.h"
#include "devicecontroller.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 初始化设备控制器
    DeviceController::instance();
    
    MainWindow w;
    w.resize(1280, 680);
    w.showFullScreen();
    w.show();
    return a.exec();
}
