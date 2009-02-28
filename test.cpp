#include "test.h"

void Printer::print(const UsbDevice& a){
       qDebug()<<a.udi;
        qDebug()<<a.device;
        qDebug()<<a.label;
        qDebug()<<a.model;
        qDebug()<<a.mounted;
        qDebug()<<a.mount_point;
        qDebug()<<a.readonly;
        qDebug()<<endl;
}

void Printer::print2(const UsbDevice& a,const UsbDevice& b){
        qDebug()<<"Was:";
        emit print(a);
        qDebug()<<"Became:";
        emit print(b);
}
