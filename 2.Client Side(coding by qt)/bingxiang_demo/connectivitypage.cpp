#include "connectivitypage.h"
#include <QFont>
#include <QMessageBox>
#include <QDebug>
#include <QPalette>
#include <QColor>
#include <QScrollBar>

// ============ SmoothScrollListWidget 实现 ============
SmoothScrollListWidget::SmoothScrollListWidget(QWidget *parent)
    : QListWidget(parent), isDragging(false), scrollStartValue(0)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);  // 使用像素滚动模式
}

void SmoothScrollListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        scrollStartValue = verticalScrollBar()->value();
        event->accept();
    } else {
        QListWidget::mousePressEvent(event);
    }
}

void SmoothScrollListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        int deltaY = event->pos().y() - lastMousePos.y();
        
        // 直接根据鼠标移动距离滚动，1:1 映射，无惯性
        int newValue = verticalScrollBar()->value() - deltaY;
        verticalScrollBar()->setValue(newValue);
        
        lastMousePos = event->pos();
        event->accept();
    } else {
        QListWidget::mouseMoveEvent(event);
    }
}

void SmoothScrollListWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        isDragging = false;
        
        // 检查是否是点击（移动距离很小）
        int totalDelta = qAbs(event->pos().y() - lastMousePos.y());
        if (totalDelta < 5) {
            // 这是一个点击，传递给父类处理
            QListWidget::mousePressEvent(event);
            QListWidget::mouseReleaseEvent(event);
        }
        event->accept();
    } else {
        QListWidget::mouseReleaseEvent(event);
    }
}

// ============ TextKeyboard 实现 ============
TextKeyboard::TextKeyboard(const QString &title, QWidget *parent)
    : QDialog(parent), isUpperCase(false)
{
    setupUI(title);
    setModal(true);
    setWindowTitle(title);
}

void TextKeyboard::setupUI(const QString &title)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    this->setStyleSheet("QDialog { background-color: #F5F5F5; }");
    this->setMinimumSize(1000, 700);  // 从 700x500 增加到 1000x700

    // 标题
    QLabel *titleLabel = new QLabel(title);
    QFont titleFont;
    titleFont.setPointSize(24);  // 从 20 增加到 24
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #333; padding: 15px;");

    // 显示区域
    displayEdit = new QLineEdit();
    displayEdit->setReadOnly(true);
    displayEdit->setMinimumHeight(70);  // 从 50 增加到 70
    QFont displayFont;
    displayFont.setPointSize(22);  // 从 18 增加到 22
    displayEdit->setFont(displayFont);
    displayEdit->setStyleSheet(
        "QLineEdit { "
        "background-color: white; "
        "border: 3px solid #00BCD4; "
        "border-radius: 8px; "
        "padding: 15px; "
        "color: #333; "
        "}"
    );

    // 键盘布局
    QGridLayout *keypadLayout = new QGridLayout();
    keypadLayout->setSpacing(12);  // 从 8 增加到 12

    QString buttonStyle = 
        "QPushButton { "
        "background-color: white; "
        "border: 2px solid #E0E0E0; "
        "border-radius: 8px; "
        "font-size: 22px; "  // 从 16 增加到 22
        "font-weight: bold; "
        "color: #333; "
        "min-height: 65px; "  // 从 45 增加到 65
        "min-width: 70px; "  // 从 50 增加到 70
        "}"
        "QPushButton:pressed { "
        "background-color: #E0E0E0; "
        "}";

    // 第一行: 数字
    QString row1 = "1234567890";
    for (int i = 0; i < row1.length(); i++) {
        QPushButton *btn = new QPushButton(QString(row1[i]));
        btn->setStyleSheet(buttonStyle);
        connect(btn, &QPushButton::clicked, this, &TextKeyboard::onKeyClicked);
        keypadLayout->addWidget(btn, 0, i);
    }

    // 第二行: QWERTYUIOP
    QString row2 = "qwertyuiop";
    for (int i = 0; i < row2.length(); i++) {
        QPushButton *btn = new QPushButton(QString(row2[i]));
        btn->setStyleSheet(buttonStyle);
        connect(btn, &QPushButton::clicked, this, &TextKeyboard::onKeyClicked);
        letterButtons.append(btn);
        keypadLayout->addWidget(btn, 1, i);
    }

    // 第三行: ASDFGHJKL
    QString row3 = "asdfghjkl";
    for (int i = 0; i < row3.length(); i++) {
        QPushButton *btn = new QPushButton(QString(row3[i]));
        btn->setStyleSheet(buttonStyle);
        connect(btn, &QPushButton::clicked, this, &TextKeyboard::onKeyClicked);
        letterButtons.append(btn);
        keypadLayout->addWidget(btn, 2, i);
    }

    // 第四行: Shift + ZXCVBNM + Backspace
    QPushButton *shiftBtn = new QPushButton("⇧");
    shiftBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #FFC107; "
        "border: none; "
        "border-radius: 8px; "
        "font-size: 28px; "  // 从 20 增加到 28
        "font-weight: bold; "
        "color: white; "
        "min-height: 65px; "  // 从 45 增加到 65
        "}"
        "QPushButton:pressed { "
        "background-color: #FFA000; "
        "}"
    );
    connect(shiftBtn, &QPushButton::clicked, this, &TextKeyboard::onShiftClicked);
    keypadLayout->addWidget(shiftBtn, 3, 0);

    QString row4 = "zxcvbnm";
    for (int i = 0; i < row4.length(); i++) {
        QPushButton *btn = new QPushButton(QString(row4[i]));
        btn->setStyleSheet(buttonStyle);
        connect(btn, &QPushButton::clicked, this, &TextKeyboard::onKeyClicked);
        letterButtons.append(btn);
        keypadLayout->addWidget(btn, 3, i + 1);
    }

    QPushButton *backspaceBtn = new QPushButton("←");
    backspaceBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #FF9800; "
        "border: none; "
        "border-radius: 8px; "
        "font-size: 28px; "  // 从 20 增加到 28
        "font-weight: bold; "
        "color: white; "
        "min-height: 65px; "  // 从 45 增加到 65
        "}"
        "QPushButton:pressed { "
        "background-color: #F57C00; "
        "}"
    );
    connect(backspaceBtn, &QPushButton::clicked, this, &TextKeyboard::onBackspaceClicked);
    keypadLayout->addWidget(backspaceBtn, 3, 8, 1, 2);

    // 第五行: 特殊字符和空格
    QString specialChars = "@#$%&*_-+=";
    for (int i = 0; i < specialChars.length(); i++) {
        QPushButton *btn = new QPushButton(QString(specialChars[i]));
        btn->setStyleSheet(buttonStyle);
        connect(btn, &QPushButton::clicked, this, &TextKeyboard::onKeyClicked);
        keypadLayout->addWidget(btn, 4, i);
    }

    // 第六行: 清除和空格
    QPushButton *clearBtn = new QPushButton("清除");
    clearBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #FF5722; "
        "border: none; "
        "border-radius: 8px; "
        "font-size: 20px; "  // 从 14 增加到 20
        "font-weight: bold; "
        "color: white; "
        "min-height: 65px; "  // 从 45 增加到 65
        "}"
        "QPushButton:pressed { "
        "background-color: #E64A19; "
        "}"
    );
    connect(clearBtn, &QPushButton::clicked, this, &TextKeyboard::onClearClicked);
    keypadLayout->addWidget(clearBtn, 5, 0, 1, 2);

    QPushButton *spaceBtn = new QPushButton("空格");
    spaceBtn->setStyleSheet(buttonStyle);
    connect(spaceBtn, &QPushButton::clicked, [this]() {
        inputText += " ";
        updateDisplay();
    });
    keypadLayout->addWidget(spaceBtn, 5, 2, 1, 6);

    QPushButton *dotBtn = new QPushButton(".");
    dotBtn->setStyleSheet(buttonStyle);
    connect(dotBtn, &QPushButton::clicked, this, &TextKeyboard::onKeyClicked);
    keypadLayout->addWidget(dotBtn, 5, 8, 1, 2);

    // 底部按钮
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(20);  // 从 15 增加到 20

    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setMinimumHeight(70);  // 从 50 增加到 70
    cancelBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #9E9E9E; "
        "border: none; "
        "border-radius: 10px; "
        "font-size: 22px; "  // 从 18 增加到 22
        "font-weight: bold; "
        "color: white; "
        "}"
        "QPushButton:pressed { "
        "background-color: #757575; "
        "}"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &TextKeyboard::onCancelClicked);

    confirmBtn = new QPushButton("确认");
    confirmBtn->setMinimumHeight(70);  // 从 50 增加到 70
    confirmBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #4CAF50; "
        "border: none; "
        "border-radius: 10px; "
        "font-size: 22px; "  // 从 18 增加到 22
        "font-weight: bold; "
        "color: white; "
        "}"
        "QPushButton:pressed { "
        "background-color: #388E3C; "
        "}"
    );
    connect(confirmBtn, &QPushButton::clicked, this, &TextKeyboard::onConfirmClicked);

    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(confirmBtn);

    // 组装布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(displayEdit);
    mainLayout->addLayout(keypadLayout);
    mainLayout->addLayout(bottomLayout);
}

void TextKeyboard::onKeyClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        inputText += btn->text();
        updateDisplay();
    }
}

void TextKeyboard::onBackspaceClicked()
{
    if (!inputText.isEmpty()) {
        inputText.chop(1);
        updateDisplay();
    }
}

void TextKeyboard::onClearClicked()
{
    inputText.clear();
    updateDisplay();
}

void TextKeyboard::onShiftClicked()
{
    isUpperCase = !isUpperCase;
    updateKeyboardCase();
}

void TextKeyboard::onConfirmClicked()
{
    accept();
}

void TextKeyboard::onCancelClicked()
{
    reject();
}

void TextKeyboard::updateDisplay()
{
    displayEdit->setText(inputText);
}

void TextKeyboard::updateKeyboardCase()
{
    for (QPushButton *btn : letterButtons) {
        QString text = btn->text();
        if (isUpperCase) {
            btn->setText(text.toUpper());
        } else {
            btn->setText(text.toLower());
        }
    }
}

// ============ ConnectivityPage 实现 ============
ConnectivityPage::ConnectivityPage(QWidget *parent)
    : QWidget(parent),
      scanProcess(nullptr),
      connectProcess(nullptr),
      disconnectProcess(nullptr),
      statusProcess(nullptr),
      ipProcess(nullptr)
{
    // 设置整个页面的背景色
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#2D2D2D"));
    setPalette(pal);
    
    setupUI();
    
    // 创建定时器更新状态
    wifiStatusTimer = new QTimer(this);
    connect(wifiStatusTimer, &QTimer::timeout, this, &ConnectivityPage::updateWifiStatus);
    wifiStatusTimer->start(5000); // 每5秒更新一次
    
    // 初始更新状态
    updateWifiStatus();
}

ConnectivityPage::~ConnectivityPage()
{
    // 清理进程
    if (scanProcess) scanProcess->deleteLater();
    if (connectProcess) connectProcess->deleteLater();
    if (disconnectProcess) disconnectProcess->deleteLater();
    if (statusProcess) statusProcess->deleteLater();
    if (ipProcess) ipProcess->deleteLater();
}

void ConnectivityPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 顶部布局：标题在左，状态信息在右
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(20);
    
    // 左侧：标题
    QLabel *titleLabel = new QLabel("WiFi 网络设置");
    QFont titleFont;
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet("color: #00BCD4; padding: 10px; background-color: transparent;");
    
    // 右侧：状态信息垂直布局
    QVBoxLayout *statusLayout = new QVBoxLayout();
    statusLayout->setSpacing(10);
    
    setupWifiPanel();
    
    statusLayout->addWidget(wifiStatusLabel);
    statusLayout->addWidget(wifiIpLabel);
    
    topLayout->addWidget(titleLabel, 1);  // 标题占1份
    topLayout->addLayout(statusLayout, 1);  // 状态信息占1份
    
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(wifiListWidget);
    mainLayout->addWidget(wifiPasswordEdit);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(15);
    btnLayout->addWidget(wifiScanBtn);
    btnLayout->addWidget(wifiConnectBtn);
    btnLayout->addWidget(wifiDisconnectBtn);
    mainLayout->addLayout(btnLayout);
}

void ConnectivityPage::setupWifiPanel()
{
    // WiFi状态
    wifiStatusLabel = new QLabel("状态: 未连接");
    wifiStatusLabel->setStyleSheet(
        "QLabel { "
        "background-color: #2D2D2D; "
        "color: #FFFFFF; "
        "font-size: 18px; "
        "padding: 15px; "
        "border-radius: 8px; "
        "}"
    );
    wifiStatusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    // IP地址显示
    wifiIpLabel = new QLabel("IP地址: --");
    wifiIpLabel->setStyleSheet(
        "QLabel { "
        "background-color: #2D2D2D; "
        "color: #4CAF50; "
        "font-size: 18px; "
        "padding: 15px; "
        "border-radius: 8px; "
        "}"
    );
    wifiIpLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    // WiFi列表
    wifiListWidget = new SmoothScrollListWidget();
    
    wifiListWidget->setStyleSheet(
        "QListWidget { "
        "background-color: #2D2D2D; "
        "color: #FFFFFF; "
        "border: 2px solid #00BCD4; "
        "border-radius: 8px; "
        "font-size: 22px; "  // 增大字体从16到22
        "padding: 5px; "
        "}"
        "QListWidget::item { "
        "padding: 20px; "  // 增大内边距从12到20
        "border-bottom: 1px solid #3D3D3D; "
        "min-height: 50px; "  // 设置最小高度
        "}"
        "QListWidget::item:selected { "
        "background-color: #00BCD4; "
        "color: white; "
        "}"
        "QListWidget::item:hover { "
        "background-color: #3D3D3D; "
        "}"
        "QScrollBar:vertical { "
        "background-color: #2D2D2D; "
        "width: 12px; "
        "border-radius: 6px; "
        "}"
        "QScrollBar::handle:vertical { "
        "background-color: #00BCD4; "
        "border-radius: 6px; "
        "min-height: 30px; "
        "}"
        "QScrollBar::handle:vertical:hover { "
        "background-color: #00ACC1; "
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "height: 0px; "
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { "
        "background: none; "
        "}"
    );
    connect(wifiListWidget, &QListWidget::itemClicked, this, &ConnectivityPage::onWifiItemClicked);
    
    // 密码输入
    wifiPasswordEdit = new QLineEdit();
    wifiPasswordEdit->setPlaceholderText("点击输入WiFi密码");
    wifiPasswordEdit->setReadOnly(true);
    wifiPasswordEdit->setEchoMode(QLineEdit::Password);
    wifiPasswordEdit->setStyleSheet(
        "QLineEdit { "
        "background-color: #2D2D2D; "
        "color: #FFFFFF; "
        "border: 2px solid #00BCD4; "
        "border-radius: 8px; "
        "padding: 15px; "
        "font-size: 16px; "
        "}"
    );
    // 使用installEventFilter来捕获点击事件
    wifiPasswordEdit->installEventFilter(this);
    
    // 按钮
    wifiScanBtn = new QPushButton("扫描网络");
    wifiConnectBtn = new QPushButton("连接");
    wifiDisconnectBtn = new QPushButton("断开连接");
    
    QList<QPushButton*> buttons = {wifiScanBtn, wifiConnectBtn, wifiDisconnectBtn};
    for (auto btn : buttons) {
        btn->setMinimumHeight(60);
        btn->setStyleSheet(
            "QPushButton { "
            "background-color: #00BCD4; "
            "color: white; "
            "border: none; "
            "border-radius: 8px; "
            "font-size: 18px; "
            "font-weight: bold; "
            "}"
            "QPushButton:hover { "
            "background-color: #00ACC1; "
            "}"
            "QPushButton:pressed { "
            "background-color: #0097A7; "
            "}"
        );
    }
    
    connect(wifiScanBtn, &QPushButton::clicked, this, &ConnectivityPage::onWifiScanClicked);
    connect(wifiConnectBtn, &QPushButton::clicked, this, &ConnectivityPage::onWifiConnectClicked);
    connect(wifiDisconnectBtn, &QPushButton::clicked, this, &ConnectivityPage::onWifiDisconnectClicked);
}

bool ConnectivityPage::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == wifiPasswordEdit && event->type() == QEvent::MouseButtonPress) {
        onPasswordEditClicked();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

// WiFi槽函数
void ConnectivityPage::onWifiScanClicked()
{
    wifiListWidget->clear();
    wifiScanBtn->setEnabled(false);
    wifiScanBtn->setText("扫描中...");
    
    scanWifiNetworks();
}

void ConnectivityPage::onWifiConnectClicked()
{
    if (selectedWifiSsid.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一个WiFi网络");
        return;
    }
    
    QString password = wifiPasswordEdit->text();
    
    // 禁用按钮防止重复点击
    wifiConnectBtn->setEnabled(false);
    wifiConnectBtn->setText("连接中...");
    
    connectToWifi(selectedWifiSsid, password);
}

void ConnectivityPage::onWifiDisconnectClicked()
{
    wifiDisconnectBtn->setEnabled(false);
    wifiDisconnectBtn->setText("断开中...");
    disconnectWifi();
}

void ConnectivityPage::onWifiItemClicked(QListWidgetItem *item)
{
    selectedWifiSsid = item->text();
}

void ConnectivityPage::onPasswordEditClicked()
{
    TextKeyboard keyboard("输入WiFi密码", this);
    if (keyboard.exec() == QDialog::Accepted) {
        wifiPasswordEdit->setText(keyboard.getText());
    }
}

// 状态更新
void ConnectivityPage::updateWifiStatus()
{
    requestWifiStatus();
    requestIpAddress();
}

// WiFi辅助函数
void ConnectivityPage::scanWifiNetworks()
{
    // 如果已有扫描进程在运行，先清理
    if (scanProcess) {
        scanProcess->disconnect();
        scanProcess->deleteLater();
    }
    
    scanProcess = new QProcess(this);
    connect(scanProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ConnectivityPage::onScanProcessFinished);
    
    scanProcess->start("nmcli", QStringList() << "device" << "wifi" << "list");
}

void ConnectivityPage::onScanProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    
    if (exitCode == 0 && scanProcess) {
        QString output = scanProcess->readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        for (int i = 1; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (!line.isEmpty()) {
                // 解析SSID（简化版本）
                QStringList parts = line.split(QRegExp("\\s+"));
                if (parts.size() > 1) {
                    QString ssid = parts[1];
                    if (ssid != "--" && !ssid.isEmpty() && ssid != "*") {
                        // 检查信号强度
                        QString signal = "";
                        for (int j = 0; j < parts.size(); ++j) {
                            if (parts[j].endsWith("%")) {
                                signal = " (" + parts[j] + ")";
                                break;
                            }
                        }
                        wifiListWidget->addItem(ssid + signal);
                    }
                }
            }
        }
    }
    
    wifiScanBtn->setEnabled(true);
    wifiScanBtn->setText("扫描网络");
    
    if (scanProcess) {
        scanProcess->deleteLater();
        scanProcess = nullptr;
    }
}

void ConnectivityPage::connectToWifi(const QString &ssid, const QString &password)
{
    // 从显示文本中提取SSID（去掉信号强度）
    QString cleanSsid = ssid;
    int bracketPos = cleanSsid.indexOf(" (");
    if (bracketPos > 0) {
        cleanSsid = cleanSsid.left(bracketPos);
    }
    
    // 如果已有连接进程在运行，先清理
    if (connectProcess) {
        connectProcess->disconnect();
        connectProcess->deleteLater();
    }
    
    connectProcess = new QProcess(this);
    connect(connectProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ConnectivityPage::onConnectProcessFinished);
    
    QStringList args;
    args << "device" << "wifi" << "connect" << cleanSsid;
    if (!password.isEmpty()) {
        args << "password" << password;
    }
    
    connectProcess->start("nmcli", args);
}

void ConnectivityPage::onConnectProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    
    wifiConnectBtn->setEnabled(true);
    wifiConnectBtn->setText("连接");
    
    if (exitCode == 0) {
        updateWifiStatus();
        QMessageBox::information(this, "成功", "WiFi连接成功");
    } else if (connectProcess) {
        QString error = connectProcess->readAllStandardError();
        QMessageBox::warning(this, "失败", "WiFi连接失败\n" + error);
    }
    
    if (connectProcess) {
        connectProcess->deleteLater();
        connectProcess = nullptr;
    }
}

void ConnectivityPage::disconnectWifi()
{
    // 如果已有断开进程在运行，先清理
    if (disconnectProcess) {
        disconnectProcess->disconnect();
        disconnectProcess->deleteLater();
    }
    
    disconnectProcess = new QProcess(this);
    connect(disconnectProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ConnectivityPage::onDisconnectProcessFinished);
    
    disconnectProcess->start("nmcli", QStringList() << "device" << "disconnect" << "wlan0");
}

void ConnectivityPage::onDisconnectProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    
    wifiDisconnectBtn->setEnabled(true);
    wifiDisconnectBtn->setText("断开连接");
    
    if (exitCode == 0) {
        QMessageBox::information(this, "成功", "WiFi已断开");
        updateWifiStatus();
    }
    
    if (disconnectProcess) {
        disconnectProcess->deleteLater();
        disconnectProcess = nullptr;
    }
}

void ConnectivityPage::requestWifiStatus()
{
    // 如果已有状态查询进程在运行，跳过本次查询
    if (statusProcess && statusProcess->state() == QProcess::Running) {
        return;
    }
    
    if (statusProcess) {
        statusProcess->disconnect();
        statusProcess->deleteLater();
    }
    
    statusProcess = new QProcess(this);
    connect(statusProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ConnectivityPage::onStatusProcessFinished);
    
    statusProcess->start("nmcli", QStringList() << "-t" << "-f" << "active,ssid" << "dev" << "wifi");
}

void ConnectivityPage::onStatusProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    
    if (exitCode == 0 && statusProcess) {
        QString output = statusProcess->readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        bool connected = false;
        for (const QString &line : lines) {
            if (line.startsWith("yes:")) {
                wifiStatusLabel->setText("状态: 已连接 - " + line.mid(4));
                connected = true;
                break;
            }
        }
        
        if (!connected) {
            wifiStatusLabel->setText("状态: 未连接");
        }
    }
    
    if (statusProcess) {
        statusProcess->deleteLater();
        statusProcess = nullptr;
    }
}

void ConnectivityPage::requestIpAddress()
{
    // 如果已有IP查询进程在运行，跳过本次查询
    if (ipProcess && ipProcess->state() == QProcess::Running) {
        return;
    }
    
    if (ipProcess) {
        ipProcess->disconnect();
        ipProcess->deleteLater();
    }
    
    ipProcess = new QProcess(this);
    connect(ipProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ConnectivityPage::onIpProcessFinished);
    
    ipProcess->start("ip", QStringList() << "addr" << "show" << "wlan0");
}

void ConnectivityPage::onIpProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    
    if (exitCode == 0 && ipProcess) {
        QString output = ipProcess->readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        bool found = false;
        for (const QString &line : lines) {
            if (line.contains("inet ") && !line.contains("inet6")) {
                QStringList parts = line.trimmed().split(' ');
                if (parts.size() >= 2) {
                    QString ip = parts[1];
                    // 移除子网掩码部分
                    int slashPos = ip.indexOf('/');
                    if (slashPos > 0) {
                        ip = ip.left(slashPos);
                    }
                    wifiIpLabel->setText("IP地址: " + ip);
                    found = true;
                    break;
                }
            }
        }
        
        if (!found) {
            wifiIpLabel->setText("IP地址: --");
        }
    }
    
    if (ipProcess) {
        ipProcess->deleteLater();
        ipProcess = nullptr;
    }
}
