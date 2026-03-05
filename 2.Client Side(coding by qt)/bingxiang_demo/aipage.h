#ifndef AIPAGE_H
#define AIPAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTcpSocket>
#include <QScrollArea>
#include <QTimer>
#include <QMouseEvent>

// 自定义滚动区域，支持触摸滑动
class SmoothScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    explicit SmoothScrollArea(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool isDragging;
    QPoint lastMousePos;
    int scrollStartValue;
};

class AIPage : public QWidget
{
    Q_OBJECT

public:
    explicit AIPage(QWidget *parent = nullptr);
    ~AIPage();

private:
    QLabel *titleLabel;
    SmoothScrollArea *scrollArea;
    QWidget *chatContainer;
    QVBoxLayout *chatLayout;
    
    QTcpSocket *tcpSocket;
    QString receiveBuffer;

    void setupUI();
    void connectToServer();
    void addUserMessage(const QString &message);
    void addAssistantMessage(const QString &message);
    void addSystemMessage(const QString &message);

private slots:
    void onSendClicked();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpReadyRead();
    void onTcpError(QAbstractSocket::SocketError error);
};

#endif // AIPAGE_H
