
#include <QtCore>
#include <QApplication>
#include <iostream>
#include <QObject>
using namespace std;

#include "halobject.h" 
#include "test.h"



int main(int argc, char** argv) 
{
    //Создаём объектики и связываем их по слотам-сигналам
    HalObject * halObj = new HalObject(); 
    Printer * p=new Printer();
    QObject::connect(halObj, SIGNAL(deviceAdded(UsbDevice)),p,
                      SLOT(print(UsbDevice)));
    
    QObject::connect(halObj, SIGNAL(deviceRemoved(UsbDevice)),p,
                      SLOT(print(UsbDevice)));

    QObject::connect(halObj, SIGNAL(deviceModificated(UsbDevice,UsbDevice)),p,
                      SLOT(print2(UsbDevice,UsbDevice)));
    
    //Вывод уже подключенных устройств
    foreach (UsbDevice a,halObj->listDevices()){
        qDebug()<<a.udi;
        qDebug()<<a.device;
        qDebug()<<a.label;
        qDebug()<<a.model;
        qDebug()<<a.mounted;
        qDebug()<<a.mount_point;
        qDebug()<<a.readonly;
        qDebug()<<endl;
    }
    QApplication app(argc,argv); 
    return app.exec(); 
}
