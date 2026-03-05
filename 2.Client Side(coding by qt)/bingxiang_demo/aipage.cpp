#include "aipage.h"
#include <QFont>
#include <QDateTime>
#include <QScrollBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

// ============ SmoothScrollArea 实现 ============
SmoothScrollArea::SmoothScrollArea(QWidget *parent)
    : QScrollArea(parent), isDragging(false), scrollStartValue(0)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void SmoothScrollArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        scrollStartValue = verticalScrollBar()->value();
        event->accept();
    } else {
        QScrollArea::mousePressEvent(event);
    }
}

void SmoothScrollArea::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        int deltaY = event->pos().y() - lastMousePos.y();
        
        // 直接根据鼠标移动距离滚动
        int newValue = verticalScrollBar()->value() - deltaY;
        verticalScrollBar()->setValue(newValue);
        
        lastMousePos = event->pos();
        event->accept();
    } else {
        QScrollArea::mouseMoveEvent(event);
    }
}

void SmoothScrollArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        isDragging = false;
        event->accept();
    } else {
        QScrollArea::mouseReleaseEvent(event);
    }
}

// ============ AIPage 实现 ============
AIPage::AIPage(QWidget *parent)
    : QWidget(parent), tcpSocket(nullptr)
{
    setupUI();
    connectToServer();
}

AIPage::~AIPage()
{
    if (tcpSocket) {
        tcpSocket->disconnectFromHost();
        tcpSocket->deleteLater();
    }
}

void AIPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    this->setStyleSheet("background-color: #EDEDED;");

    // 标题
    titleLabel = new QLabel("AI 智能助手");
    QFont titleFont;
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #4CAF50; padding: 10px; background-color: white;");

    // 聊天显示区域（滚动区域）
    scrollArea = new SmoothScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #EDEDED; }");
    
    chatContainer = new QWidget();
    chatLayout = new QVBoxLayout(chatContainer);
    chatLayout->setSpacing(10);
    chatLayout->setContentsMargins(10, 10, 10, 10);
    chatLayout->addStretch();
    
    scrollArea->setWidget(chatContainer);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(scrollArea, 1);

    // 添加欢迎消息
    addSystemMessage("欢迎使用AI智能助手！请说\"你好\"激活对话。");
}

void AIPage::onSendClicked()
{
    // 此函数已不再需要，因为没有输入框了
}

void AIPage::connectToServer()
{
    tcpSocket = new QTcpSocket(this);
    
    connect(tcpSocket, &QTcpSocket::connected, this, &AIPage::onTcpConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &AIPage::onTcpDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &AIPage::onTcpReadyRead);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &AIPage::onTcpError);
    
    // 连接到Python服务器
    tcpSocket->connectToHost("127.0.0.1", 9999);
}

void AIPage::onTcpConnected()
{
    qDebug() << "已连接到AI服务器";
    addSystemMessage("已连接到AI服务器");
}

void AIPage::onTcpDisconnected()
{
    qDebug() << "与AI服务器断开连接";
    addSystemMessage("与AI服务器断开连接，正在重连...");
    
    // 3秒后重连
    QTimer::singleShot(3000, this, [this]() {
        tcpSocket->connectToHost("127.0.0.1", 9999);
    });
}

void AIPage::onTcpReadyRead()
{
    // 读取数据
    QByteArray data = tcpSocket->readAll();
    receiveBuffer.append(QString::fromUtf8(data));
    
    qDebug() << "[TCP接收] 收到数据，缓冲区大小:" << receiveBuffer.length();
    
    // 处理完整的JSON消息（以换行符分隔）
    int newlineIndex;
    while ((newlineIndex = receiveBuffer.indexOf('\n')) != -1) {
        QString jsonStr = receiveBuffer.left(newlineIndex).trimmed();
        receiveBuffer = receiveBuffer.mid(newlineIndex + 1);
        
        if (jsonStr.isEmpty()) continue;
        
        qDebug() << "[TCP解析] JSON字符串:" << jsonStr;
        
        // 解析JSON
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isObject()) {
            qDebug() << "[TCP解析] JSON解析失败";
            continue;
        }
        
        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        QString content = obj["content"].toString();
        
        qDebug() << "[TCP处理] 类型:" << type << "内容长度:" << content.length();
        
        if (type == "user") {
            addUserMessage(content);
        } else if (type == "assistant") {
            addAssistantMessage(content);
        } else if (type == "system") {
            addSystemMessage(content);
        }
    }
}

void AIPage::onTcpError(QAbstractSocket::SocketError error)
{
    qDebug() << "TCP错误:" << tcpSocket->errorString();
    addSystemMessage("连接错误: " + tcpSocket->errorString());
}

void AIPage::addUserMessage(const QString &message)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm");
    
    // 创建消息容器
    QWidget *msgWidget = new QWidget();
    msgWidget->setStyleSheet("background-color: #95EC69; border-radius: 18px;");
    msgWidget->setMaximumWidth(600);
    
    QVBoxLayout *msgContentLayout = new QVBoxLayout(msgWidget);
    msgContentLayout->setContentsMargins(15, 12, 15, 12);
    msgContentLayout->setSpacing(5);
    
    QLabel *textLabel = new QLabel(message);
    textLabel->setWordWrap(true);
    textLabel->setStyleSheet("background-color: transparent; color: black; font-size: 24px;");
    
    QLabel *timeLabel = new QLabel(time);
    timeLabel->setStyleSheet("background-color: transparent; color: #666; font-size: 14px;");
    timeLabel->setAlignment(Qt::AlignRight);
    
    msgContentLayout->addWidget(textLabel);
    msgContentLayout->addWidget(timeLabel);
    
    QHBoxLayout *msgLayout = new QHBoxLayout();
    msgLayout->addStretch();
    msgLayout->addWidget(msgWidget);
    
    chatLayout->insertLayout(chatLayout->count() - 1, msgLayout);
    
    // 滚动到底部
    QTimer::singleShot(100, this, [this]() {
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    });
}

void AIPage::addAssistantMessage(const QString &message)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm");
    
    // 创建消息容器
    QWidget *msgWidget = new QWidget();
    msgWidget->setStyleSheet("background-color: white; border-radius: 18px;");
    msgWidget->setMaximumWidth(600);
    
    QVBoxLayout *msgContentLayout = new QVBoxLayout(msgWidget);
    msgContentLayout->setContentsMargins(15, 12, 15, 12);
    msgContentLayout->setSpacing(5);
    
    QLabel *textLabel = new QLabel(message);
    textLabel->setWordWrap(true);
    textLabel->setStyleSheet("background-color: transparent; color: black; font-size: 26px;");
    
    QLabel *timeLabel = new QLabel(time);
    timeLabel->setStyleSheet("background-color: transparent; color: #666; font-size: 14px;");
    timeLabel->setAlignment(Qt::AlignLeft);
    
    msgContentLayout->addWidget(textLabel);
    msgContentLayout->addWidget(timeLabel);
    
    QHBoxLayout *msgLayout = new QHBoxLayout();
    msgLayout->addWidget(msgWidget);
    msgLayout->addStretch();
    
    chatLayout->insertLayout(chatLayout->count() - 1, msgLayout);
    
    // 滚动到底部
    QTimer::singleShot(100, this, [this]() {
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    });
}

void AIPage::addSystemMessage(const QString &message)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm");
    
    QLabel *msgLabel = new QLabel();
    msgLabel->setWordWrap(true);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setText(QString("<span style='font-size: 12px; color: #999;'>%1 %2</span>")
                      .arg(message).arg(time));
    msgLabel->setStyleSheet("background-color: transparent; padding: 5px;");
    
    chatLayout->insertWidget(chatLayout->count() - 1, msgLabel);
    
    // 滚动到底部
    QTimer::singleShot(100, this, [this]() {
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    });
}
