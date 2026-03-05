#ifndef CONNECTIVITYPAGE_H
#define CONNECTIVITYPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QProcess>
#include <QTimer>
#include <QDialog>
#include <QGridLayout>
#include <QEvent>
#include <QMouseEvent>

#include <QEvent>
#include <QMouseEvent>

// 自定义 ListWidget 支持平滑拖动滚动
class SmoothScrollListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit SmoothScrollListWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool isDragging;
    QPoint lastMousePos;
    int scrollStartValue;
};

class TextKeyboard : public QDialog
{
    Q_OBJECT

public:
    explicit TextKeyboard(const QString &title, QWidget *parent = nullptr);
    QString getText() const { return inputText; }

private slots:
    void onKeyClicked();
    void onBackspaceClicked();
    void onClearClicked();
    void onConfirmClicked();
    void onCancelClicked();
    void onShiftClicked();

private:
    void setupUI(const QString &title);
    void updateDisplay();
    void updateKeyboardCase();

    QLineEdit *displayEdit;
    QPushButton *confirmBtn;
    QList<QPushButton*> letterButtons;
    
    QString inputText;
    bool isUpperCase;
};

class ConnectivityPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectivityPage(QWidget *parent = nullptr);
    ~ConnectivityPage();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onWifiScanClicked();
    void onWifiConnectClicked();
    void onWifiDisconnectClicked();
    void onWifiItemClicked(QListWidgetItem *item);
    void onPasswordEditClicked();
    
    void updateWifiStatus();
    
    // 异步处理槽函数
    void onScanProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onConnectProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDisconnectProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onStatusProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onIpProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUI();
    void setupWifiPanel();
    
    // WiFi相关
    QLabel *wifiStatusLabel;
    QLabel *wifiIpLabel;
    QPushButton *wifiScanBtn;
    QPushButton *wifiConnectBtn;
    QPushButton *wifiDisconnectBtn;
    SmoothScrollListWidget *wifiListWidget;
    QLineEdit *wifiPasswordEdit;
    QString selectedWifiSsid;
    QTimer *wifiStatusTimer;
    
    // 异步进程管理
    QProcess *scanProcess;
    QProcess *connectProcess;
    QProcess *disconnectProcess;
    QProcess *statusProcess;
    QProcess *ipProcess;
    
    // 辅助函数
    void scanWifiNetworks();
    void connectToWifi(const QString &ssid, const QString &password);
    void disconnectWifi();
    void requestWifiStatus();
    void requestIpAddress();
};

#endif // CONNECTIVITYPAGE_H
