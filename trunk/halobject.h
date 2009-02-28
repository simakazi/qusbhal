#ifndef HALOBJECT_H  
#define HALOBJECT_H  

#include <QObject> 
#include <QString> 
#include <QDebug>  
#include <QDBusInterface>

// Эта структура нужна, чтобы получать информацию об изменении свойств носителей
struct Property {
QString name;  
bool added;  
bool removed;  
}; 


// А эта - хранит информацию о носителей, с помощью таких структур происходит обмен информацией между классом и внешним миром
struct UsbDevice {
    QString udi; //Уникальный идентификатор устройства в системе Linux
    QString device; //Файл устройства
    QString label; //Метка тома
    QString mount_point; //Точка монтирования устройства
    QString model; //Тип носителя, обычно Flash, другого не видел
    bool mounted; //Факт примонтированности
    bool readonly; //очевидно
};



class HalObject : public QObject
{ 
Q_OBJECT 
private:
    QString getProperty(QDBusInterface& device,const QString& name);
    QList<UsbDevice> PlugedDevices;
    QList<UsbDevice> generateList();
public: 
    HalObject(QObject * parent=0);
    QList<UsbDevice> listDevices();
    UsbDevice getDevice(const QString& udi);
    virtual ~HalObject() {};

public slots:
    void modDevice(int num, QList<Property> prop);
    void addDevice(const QString& qwe);
    void delDevice(const QString& qwe);

signals:
    void deviceAdded(const UsbDevice& device); // Эти два сигнала, думаю, понятны - шлют структуру, содержащую в себе информацию
    void deviceRemoved(const UsbDevice& device);//о том, что за устройство было подключено/отключено
    void deviceModificated(const UsbDevice& was,const UsbDevice& now); //Первый параметр - то что было, второй - то, что стало с устройством
};

#endif
