#include "halobject.h"

#include <QDBusMessage>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QtDBus>

//Макросы - нужны для связывания самописных слотов с сигналами DBus
Q_DECLARE_METATYPE(Property)
Q_DECLARE_METATYPE(QList<Property>)

// Ниже два оператора - опять требуются дбасу
const QDBusArgument& operator<<(QDBusArgument &arg, const Property &change) 
{
arg.beginStructure(); 
arg << change.name << change.added << change.removed; 
arg.endStructure(); 
return arg; 
}

const QDBusArgument& operator>>(const QDBusArgument &arg, Property &change) 
{
arg.beginStructure(); 
arg >> change.name >> change.added >> change.removed; 
arg.endStructure(); 
return arg; 
}

HalObject::HalObject(QObject * parent){
    // Конструктор регистрирует созданные уже метатипы
    qDBusRegisterMetaType<Property>(); 
    qDBusRegisterMetaType<QList<Property> >();

    // Инициализирует список уже подлюченных устройств 
    PlugedDevices=QList<UsbDevice>();
   
    //Подсоединяется к сигналам подключения/отключения устройств
   QDBusConnection conn = QDBusConnection::systemBus(); 
   QDBusInterface device("org.freedesktop.Hal", "/org/freedesktop/Hal/Manager", "org.freedesktop.Hal.Manager", conn);

    bool success = device.connection().connect("org.freedesktop.Hal", 
        "/org/freedesktop/Hal/Manager",
        "org.freedesktop.Hal.Manager", 
        "DeviceAdded",
        this, 
        SLOT(addDevice(QString)));

    success = device.connection().connect("org.freedesktop.Hal", 
        "/org/freedesktop/Hal/Manager",
        "org.freedesktop.Hal.Manager", 
        "DeviceRemoved",
        this, 
        SLOT(delDevice(QString)));

    // И собирает данные о уже подключенных юсб-устройствах
    PlugedDevices=generateList();
}

QString HalObject::getProperty(QDBusInterface& device,const QString& name){
    //фукнция служит для получения свойства устройства от хала
    //Гадит в дебаг всяким мусором, если не нравится - можно спокойно закоментировать
    QDBusMessage msg = device.call("GetProperty", name); 
    switch(msg.type()){ 
        case QDBusMessage::MethodCallMessage: 
            qDebug() << "Method Call Return: " << msg; 
            return "";
            break;
        case QDBusMessage::ReplyMessage: 
            //А вот эту строчку комментировать не стоит
            return msg.arguments()[0].toString();
            return "";
            break;
        case QDBusMessage::ErrorMessage: 
            qDebug() << "Error Message: " << msg.errorMessage(); 
            return "";
            break;
        case QDBusMessage::InvalidMessage: 
            qDebug() << "Invalid Message: " << msg; 
            return "";
            break;
        case QDBusMessage::SignalMessage: 
            qDebug() << "Signal Message: " << msg; 
            return "";
            break; 
    }
    return "";
}

UsbDevice HalObject::getDevice(const QString& udi){
    // Эта функция собирает данные о юсб-носителе по его UDI
    UsbDevice dev;
    QDBusConnection conn = QDBusConnection::systemBus(); 
    QDBusInterface device("org.freedesktop.Hal", udi, "org.freedesktop.Hal.Device", conn);

    // Данные об отцовском устройстве (в нашем случае - устройство юсб) используются, чтобы отличить юсб-носитель от, например, САТА-диска
    QString parent_udi = getProperty(device, "info.parent");
    QDBusInterface parent("org.freedesktop.Hal", parent_udi, "org.freedesktop.Hal.Device", conn);

    dev.udi=udi;
    dev.model=getProperty(parent,"storage.model");
    dev.device=getProperty(device,"block.device");
    dev.label=getProperty(device,"info.product");
    dev.mount_point=getProperty(device,"volume.mount_point");
    dev.mounted=(getProperty(device,"volume.is_mounted")=="true");
    dev.readonly=(getProperty(device,"volume.is_mounted_read_only")=="true");
    return dev;
}

void HalObject::addDevice(const QString& udi){
    // Этот слот используется для обработки события "добавление устройства"
    QDBusConnection conn = QDBusConnection::systemBus(); 
    QDBusInterface device("org.freedesktop.Hal", udi, "org.freedesktop.Hal.Device", conn);
    if (getProperty(device,"info.category")=="volume"){
        QString parent_udi = getProperty(device, "info.parent");
        QDBusInterface parent("org.freedesktop.Hal", parent_udi, "org.freedesktop.Hal.Device", conn);
        if (getProperty(parent,"storage.bus")=="usb"){
            UsbDevice dev=getDevice(udi);
            PlugedDevices.append(dev);
            // Вешаемся на прерывание по изменению параметров устройства (изменяется только факт монтирования)
            bool success = device.connection().connect("org.freedesktop.Hal", 
                udi,
                "org.freedesktop.Hal.Device", 
                "PropertyModified",
                this, 
                SLOT(modDevice(int,QList<Property>)));
                emit deviceAdded(dev);
        }
    }
}

void HalObject::delDevice(const QString& udi){
    // Будет вызвано при выдёргивании устройства из порта
    // Так как при этом данные устройства получить уже не возможно - имеем UDI устройства, которое не подключено,то
    // приходится изворачиваться - проходить по уже созданному списку и удалять из него отключенный диск.
    int size=PlugedDevices.size();
    for (int i=0;i<size;i++)
        if (PlugedDevices[i].udi==udi){
           emit deviceRemoved(PlugedDevices.takeAt(i));
           return;
        }
}

void HalObject::modDevice(int num, QList<Property> prop){
    //Вызывается при отмонтировании/монтировании флешки
    //Не знаю, почему так классно сделано, но данных о том, какое именно устройство изменилось, не предоставляется
    //Приходится опять перепроверять все устройства и при нахождении отличий поднимать сигнал.
    int size=PlugedDevices.size();
    foreach(UsbDevice dev0,listDevices())
    for (int i=0;i<size;i++){
        if (PlugedDevices[i].udi==dev0.udi && (PlugedDevices[i].mounted!=dev0.mounted || 
                   PlugedDevices[i].label!=dev0.label || PlugedDevices[i].mount_point!=dev0.mount_point ||  PlugedDevices[i].readonly!=dev0.readonly)){
            emit deviceModificated(PlugedDevices[i],dev0);
            PlugedDevices[i]=dev0;
        }
    }
};

QList<UsbDevice> HalObject::generateList(){
    //Генерирует список уже подключенных устройств и соединяет сигналы модификации со слотом модификации
    QList<UsbDevice> list=QList<UsbDevice>();
    QDBusConnection conn = QDBusConnection::systemBus(); 
    QDBusInterface hal("org.freedesktop.Hal","/org/freedesktop/Hal/Manager","org.freedesktop.Hal.Manager",conn);
    QDBusMessage msg = hal.call("FindDeviceByCapability", "volume"); 

    QList<QVariant> devices = msg.arguments(); 
    foreach (QVariant name, devices) 
        foreach (QString udi, name.toStringList()){
            UsbDevice dev;
            QDBusInterface device("org.freedesktop.Hal", udi, "org.freedesktop.Hal.Device", conn);

            QString parent_udi = getProperty(device, "info.parent");
            QDBusInterface parent("org.freedesktop.Hal", parent_udi, "org.freedesktop.Hal.Device", conn);
            if (getProperty(parent,"storage.bus")=="usb"){
                dev.udi=udi;
                dev.model=getProperty(parent,"storage.model");
                dev.device=getProperty(device,"block.device");
                dev.label=getProperty(device,"info.product");
                dev.mount_point=getProperty(device,"volume.mount_point");
                dev.mounted=(getProperty(device,"volume.is_mounted")=="true");
                dev.readonly=(getProperty(device,"volume.is_mounted_read_only")=="true");
                list.append(dev);

                bool success = device.connection().connect("org.freedesktop.Hal", 
                    udi,
                    "org.freedesktop.Hal.Device", 
                    "PropertyModified",
                    this, 
                    SLOT(modDevice(int,QList<Property>)));
            }
        }
    return list;
}

QList<UsbDevice> HalObject::listDevices(){
    //Этот метод нужен, чтобы силком заставить объект пересобрать информацию о носителях,
    // используется при модификации носителей
    QList<UsbDevice> list=QList<UsbDevice>();
    QDBusConnection conn = QDBusConnection::systemBus(); 
    QDBusInterface hal("org.freedesktop.Hal","/org/freedesktop/Hal/Manager","org.freedesktop.Hal.Manager",conn);
    QDBusMessage msg = hal.call("FindDeviceByCapability", "volume"); 

    QList<QVariant> devices = msg.arguments(); 
    foreach (QVariant name, devices) 
        foreach (QString udi, name.toStringList()){
            UsbDevice dev;
            QDBusInterface device("org.freedesktop.Hal", udi, "org.freedesktop.Hal.Device", conn);

            QString parent_udi = getProperty(device, "info.parent");
            QDBusInterface parent("org.freedesktop.Hal", parent_udi, "org.freedesktop.Hal.Device", conn);
            if (getProperty(parent,"storage.bus")=="usb"){
                dev.udi=udi;
                dev.model=getProperty(parent,"storage.model");
                dev.device=getProperty(device,"block.device");
                dev.label=getProperty(device,"info.product");
                dev.mount_point=getProperty(device,"volume.mount_point");
                dev.mounted=(getProperty(device,"volume.is_mounted")=="true");
                dev.readonly=(getProperty(device,"volume.is_mounted_read_only")=="true");
                list.append(dev);
           }
        }
    return list;
}