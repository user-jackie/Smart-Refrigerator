#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

class DeviceController : public QObject
{
    Q_OBJECT

public:
    static DeviceController* instance();
    
    void connectToAIServer();
    void sendStatusToAI(const QString &statusMessage);

signals:
    // 杀菌控制信号
    void sterilizationStart(int duration);
    void sterilizationStop();
    void sterilizationSetTime(int duration);
    void sterilizationQuery();
    
    // 空气净化控制信号
    void purificationStart(int duration);
    void purificationStop();
    void purificationSetTime(int duration);
    void purificationQuery();
    
    // 制冷控制信号
    void coolingStart(int temperature);
    void coolingStop();
    void coolingSetTemp(int temperature);
    
    // 制热控制信号
    void heatingStart(int temperature);
    void heatingStop();
    void heatingSetTemp(int temperature);
    
    // 温度查询信号
    void temperatureQuery();

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    explicit DeviceController(QObject *parent = nullptr);
    static DeviceController* m_instance;
    
    QTcpSocket *tcpSocket;
    QString receiveBuffer;
    
    void processControlCommand(const QJsonObject &data);
};

#endif // DEVICECONTROLLER_H
