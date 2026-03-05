#include "devicecontroller.h"
#include <QDebug>
#include <QTimer>
#include <QDateTime>

DeviceController* DeviceController::m_instance = nullptr;

DeviceController* DeviceController::instance()
{
    if (!m_instance) {
        m_instance = new DeviceController();
    }
    return m_instance;
}

DeviceController::DeviceController(QObject *parent)
    : QObject(parent), tcpSocket(nullptr)
{
    connectToAIServer();
}

void DeviceController::connectToAIServer()
{
    if (tcpSocket) {
        tcpSocket->disconnectFromHost();
        tcpSocket->deleteLater();
    }
    
    tcpSocket = new QTcpSocket(this);
    
    connect(tcpSocket, &QTcpSocket::connected, this, &DeviceController::onConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &DeviceController::onDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &DeviceController::onReadyRead);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &DeviceController::onError);
    
    // 连接到Python AI服务器
    tcpSocket->connectToHost("127.0.0.1", 9999);
}

void DeviceController::onConnected()
{
    qDebug() << "[DeviceController] 已连接到AI服务器";
}

void DeviceController::onDisconnected()
{
    qDebug() << "[DeviceController] 与AI服务器断开连接，3秒后重连...";
    
    // 3秒后重连
    QTimer::singleShot(3000, this, [this]() {
        tcpSocket->connectToHost("127.0.0.1", 9999);
    });
}

void DeviceController::onReadyRead()
{
    // 读取数据
    QByteArray data = tcpSocket->readAll();
    receiveBuffer.append(QString::fromUtf8(data));
    
    // 处理完整的JSON消息（以换行符分隔）
    int newlineIndex;
    while ((newlineIndex = receiveBuffer.indexOf('\n')) != -1) {
        QString jsonStr = receiveBuffer.left(newlineIndex).trimmed();
        receiveBuffer = receiveBuffer.mid(newlineIndex + 1);
        
        if (jsonStr.isEmpty()) continue;
        
        // 解析JSON
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isObject()) continue;
        
        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        
        // 只处理控制指令
        if (type == "control") {
            processControlCommand(obj);
        }
    }
}

void DeviceController::onError(QAbstractSocket::SocketError error)
{
    qDebug() << "[DeviceController] TCP错误:" << tcpSocket->errorString();
}

void DeviceController::sendStatusToAI(const QString &statusMessage)
{
    if (!tcpSocket || tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[DeviceController] 未连接到AI服务器，无法发送状态";
        return;
    }
    
    QJsonObject data;
    data["type"] = "status";
    data["content"] = statusMessage;
    data["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonDocument doc(data);
    QString jsonStr = doc.toJson(QJsonDocument::Compact) + "\n";
    
    tcpSocket->write(jsonStr.toUtf8());
    tcpSocket->flush();
    
    qDebug() << "[DeviceController] 发送状态到AI:" << statusMessage;
}

void DeviceController::processControlCommand(const QJsonObject &data)
{
    QString action = data["action"].toString();
    
    qDebug() << "[DeviceController] 收到控制指令:" << action;
    
    // 杀菌控制
    if (action == "sterilization_start") {
        int duration = data.value("duration").toInt(30);
        emit sterilizationStart(duration);
    } else if (action == "sterilization_stop") {
        emit sterilizationStop();
    } else if (action == "sterilization_set_time") {
        int duration = data["duration"].toInt(30);
        emit sterilizationSetTime(duration);
    } else if (action == "sterilization_query") {
        emit sterilizationQuery();
    }
    // 空气净化控制
    else if (action == "purification_start") {
        int duration = data.value("duration").toInt(30);
        emit purificationStart(duration);
    } else if (action == "purification_stop") {
        emit purificationStop();
    } else if (action == "purification_set_time") {
        int duration = data["duration"].toInt(30);
        emit purificationSetTime(duration);
    } else if (action == "purification_query") {
        emit purificationQuery();
    }
    // 制冷控制
    else if (action == "cooling_start") {
        if (data.contains("temperature")) {
            int temperature = data["temperature"].toInt(20);
            emit coolingStart(temperature);
        } else {
            emit coolingStart(-1); // -1表示使用当前设置的温度
        }
    } else if (action == "cooling_stop") {
        emit coolingStop();
    } else if (action == "cooling_set_temp") {
        int temperature = data["temperature"].toInt(20);
        emit coolingSetTemp(temperature);
    }
    // 制热控制
    else if (action == "heating_start") {
        if (data.contains("temperature")) {
            int temperature = data["temperature"].toInt(25);
            emit heatingStart(temperature);
        } else {
            emit heatingStart(-1); // -1表示使用当前设置的温度
        }
    } else if (action == "heating_stop") {
        emit heatingStop();
    } else if (action == "heating_set_temp") {
        int temperature = data["temperature"].toInt(25);
        emit heatingSetTemp(temperature);
    }
    // 温度查询
    else if (action == "temperature_query") {
        emit temperatureQuery();
    }
}
