#ifndef TEST_H  
#define TEST_H  

#include <QtCore>
#include <QObject> 
#include <QString> 
#include <QDebug>  
#include "halobject.h"

class Printer : public QObject
 {
     Q_OBJECT
     public:
         Printer(QObject * parent=0){};
    public slots:
         //слот для печати того, что пришло при подключении/отключении устройства
         void print(const UsbDevice& a);

        //Слот для печати данных о модификации
        void print2(const UsbDevice& a,const UsbDevice& b);
 };
#endif
