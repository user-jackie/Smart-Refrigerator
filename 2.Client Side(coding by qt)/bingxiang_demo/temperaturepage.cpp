#include "temperaturepage.h"
#include "devicecontroller.h"
#include "numkeyboard.h"
#include <QFont>
#include <QProcess>
#include <QDebug>

// ========== 温度读取线程实现 ==========
TempReadThread::TempReadThread(QObject *parent)
    : QThread(parent), running(true)
{
}

TempReadThread::~TempReadThread()
{
    stopThread();
}

void TempReadThread::stopThread()
{
    QMutexLocker locker(&mutex);
    running = false;
}

void TempReadThread::run()
{
    char temp_buf0[TEMP_BUF_SIZE];
    char temp_buf1[TEMP_BUF_SIZE];
    
    while (true) {
        {
            QMutexLocker locker(&mutex);
            if (!running) break;
        }
        
        QString temp0Str = "当前: --°C";
        QString temp1Str = "当前: --°C";
        
        // 读取传感器0温度
        int ret0 = ds18b20_read_temperature(DS18B20_DEV_PATH_0, temp_buf0, TEMP_BUF_SIZE);
        if (ret0 == 0) {
            temp0Str = QString("当前: %1").arg(temp_buf0);
        }
        
        // 读取传感器1温度
        int ret1 = ds18b20_read_temperature(DS18B20_DEV_PATH_1, temp_buf1, TEMP_BUF_SIZE);
        if (ret1 == 0) {
            temp1Str = QString("当前: %1").arg(temp_buf1);
        }
        
        emit temperatureUpdated(temp0Str, temp1Str);
        
        // 休眠2秒
        QThread::sleep(2);
    }
}

// ========== 温度控制页面实现 ==========
TemperaturePage::TemperaturePage(QWidget *parent)
    : QWidget(parent), heatEnabled(false), coolEnabled(false),
      lastValidTemp0(25.0f), lastValidTemp1(10.0f),
      hasValidTemp0(false), hasValidTemp1(false),
      heatingPinState(false), coolingPinState(false)
{
    setupUI();
    
    // 启动温度读取线程
    tempThread = new TempReadThread(this);
    connect(tempThread, &TempReadThread::temperatureUpdated, 
            this, &TemperaturePage::onTemperatureUpdated);
    tempThread->start();
    
    // 连接设备控制器信号
    DeviceController *controller = DeviceController::instance();
    connect(controller, &DeviceController::heatingStart, this, [this](int temperature) {
        qDebug() << "收到制热开始指令，目标温度:" << temperature << "度";
        if (temperature > 0) {
            heatSlider->setValue(temperature);
        }
        if (!heatEnabled) {
            onHeatStartClicked();
        }
    });
    connect(controller, &DeviceController::heatingStop, this, [this]() {
        qDebug() << "收到制热停止指令";
        if (heatEnabled) {
            onHeatStopClicked();
        }
    });
    connect(controller, &DeviceController::heatingSetTemp, this, [this](int temperature) {
        qDebug() << "收到制热设置温度指令:" << temperature << "度";
        heatSlider->setValue(temperature);
    });
    
    connect(controller, &DeviceController::coolingStart, this, [this](int temperature) {
        qDebug() << "收到制冷开始指令，目标温度:" << temperature << "度";
        if (temperature > 0) {
            coolSlider->setValue(temperature);
        }
        if (!coolEnabled) {
            onCoolStartClicked();
        }
    });
    connect(controller, &DeviceController::coolingStop, this, [this]() {
        qDebug() << "收到制冷停止指令";
        if (coolEnabled) {
            onCoolStopClicked();
        }
    });
    connect(controller, &DeviceController::coolingSetTemp, this, [this](int temperature) {
        qDebug() << "收到制冷设置温度指令:" << temperature << "度";
        coolSlider->setValue(temperature);
    });
    
    connect(controller, &DeviceController::temperatureQuery, this, [this]() {
        qDebug() << "收到温度查询指令";
        QString status = QString("制冷端目标温度%1度，制热端目标温度%2度")
            .arg(coolSlider->value())
            .arg(heatSlider->value());
        
        if (hasValidTemp0 || hasValidTemp1) {
            status += QString("，当前实际温度：");
            if (hasValidTemp0) {
                status += QString("制冷端%.1f度").arg(lastValidTemp0);
            }
            if (hasValidTemp1) {
                if (hasValidTemp0) status += "，";
                status += QString("制热端%.1f度").arg(lastValidTemp1);
            }
        }
        
        DeviceController::instance()->sendStatusToAI(status);
    });
}

TemperaturePage::~TemperaturePage()
{
    if (tempThread) {
        tempThread->stopThread();
        tempThread->wait(2000);
        tempThread->deleteLater();
    }
    
    // 关闭所有加热/制冷设备
    setHeatingPin(false);
    setCoolingPin(false);
}

void TemperaturePage::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 制冷区域（左半边）- 蓝色渐变
    QWidget *coolWidget = new QWidget();
    coolWidget->setStyleSheet(
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #E3F2FD, stop:0.3 #BBDEFB, stop:0.7 #90CAF9, stop:1 #64B5F6);"
        "}"
    );
    QVBoxLayout *coolLayout = new QVBoxLayout(coolWidget);
    coolLayout->setAlignment(Qt::AlignCenter);
    coolLayout->setSpacing(20);

    QFont titleFont;
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    
    coolTitleLabel = new QLabel("制冷控制");
    coolTitleLabel->setFont(titleFont);
    coolTitleLabel->setAlignment(Qt::AlignCenter);
    coolTitleLabel->setStyleSheet("color: #0D47A1; background: transparent;");

    // 实际温度显示（传感器1）
    QFont actualFont;
    actualFont.setPointSize(20);
    actualTempLabel1 = new QLabel("当前: 10.00℃");
    actualTempLabel1->setFont(actualFont);
    actualTempLabel1->setAlignment(Qt::AlignCenter);
    actualTempLabel1->setStyleSheet(
        "color: #0D47A1; "
        "background: transparent; "
        "padding: 8px; "
        "border-radius: 8px;"
    );

    QFont tempFont;
    tempFont.setPointSize(48);
    tempFont.setBold(true);
    coolTempLabel = new QLabel("10°C");
    coolTempLabel->setFont(tempFont);
    coolTempLabel->setAlignment(Qt::AlignCenter);
    coolTempLabel->setFixedWidth(220);  // 固定宽度
    coolTempLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    coolTempLabel->setStyleSheet(
        "color: #0D47A1; "
        "background: transparent; "
        "padding: 10px 20px; "
        "border-radius: 12px; "
        "border: 2px dashed #1976D2;"
    );
    coolTempLabel->setCursor(Qt::PointingHandCursor);
    coolTempLabel->installEventFilter(this);

    coolSlider = new QSlider(Qt::Vertical);
    coolSlider->setMinimum(5);
    coolSlider->setMaximum(30);
    coolSlider->setValue(10);
    coolSlider->setMinimumHeight(250);
    coolSlider->setMinimumWidth(60);
    coolSlider->setStyleSheet(
        "QSlider { background: transparent; }"
        "QSlider::groove:vertical {"
        "    background: rgba(255, 255, 255, 0.4);"
        "    width: 30px;"
        "    border-radius: 15px;"
        "    border: 2px solid rgba(25, 118, 210, 0.5);"
        "}"
        "QSlider::handle:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 #2196F3, stop:1 #1976D2);"
        "    height: 50px;"
        "    width: 50px;"
        "    margin: -10px -10px;"
        "    border-radius: 25px;"
        "    border: 3px solid white;"
        "    box-shadow: 0 2px 5px rgba(0,0,0,0.3);"
        "}"
        "QSlider::handle:vertical:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 #1E88E5, stop:1 #1565C0);"
        "    border: 3px solid #E3F2FD;"
        "}"
        "QSlider::handle:vertical:pressed {"
        "    background: #0D47A1;"
        "}"
    );

    coolStartBtn = new QPushButton("开启制冷");
    coolStartBtn->setMinimumSize(180, 70);
    coolStartBtn->setCheckable(true);
    coolStartBtn->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #666; font-size: 22px; border-radius: 10px; }"
        "QPushButton:pressed { background-color: #BDBDBD; }"
        "QPushButton:checked { background-color: #1976D2; color: white; }"
        "QPushButton:disabled { background-color: #F5F5F5; color: #BDBDBD; }"
    );
    
    coolStopBtn = new QPushButton("停止制冷");
    coolStopBtn->setMinimumSize(180, 70);
    coolStopBtn->setEnabled(false);
    coolStopBtn->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; font-size: 22px; border-radius: 10px; }"
        "QPushButton:pressed { background-color: #F57C00; }"
        "QPushButton:disabled { background-color: #F5F5F5; color: #BDBDBD; }"
    );
    
    QHBoxLayout *coolBtnLayout = new QHBoxLayout();
    coolBtnLayout->setSpacing(10);
    coolBtnLayout->addWidget(coolStartBtn);
    coolBtnLayout->addWidget(coolStopBtn);

    coolLayout->addWidget(coolTitleLabel);
    coolLayout->addWidget(actualTempLabel1);
    coolLayout->addWidget(coolTempLabel, 0, Qt::AlignCenter);
    coolLayout->addWidget(coolSlider, 0, Qt::AlignCenter);
    coolLayout->addLayout(coolBtnLayout);

    // 制热区域（右半边）- 红色渐变
    QWidget *heatWidget = new QWidget();
    heatWidget->setStyleSheet(
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #FFEBEE, stop:0.3 #FFCDD2, stop:0.7 #EF9A9A, stop:1 #E57373);"
        "}"
    );
    QVBoxLayout *heatLayout = new QVBoxLayout(heatWidget);
    heatLayout->setAlignment(Qt::AlignCenter);
    heatLayout->setSpacing(20);

    heatTitleLabel = new QLabel("制热控制");
    heatTitleLabel->setFont(titleFont);
    heatTitleLabel->setAlignment(Qt::AlignCenter);
    heatTitleLabel->setStyleSheet("color: #B71C1C; background: transparent;");

    // 实际温度显示（传感器0）
    actualTempLabel0 = new QLabel("当前: 25.00℃");
    actualTempLabel0->setFont(actualFont);
    actualTempLabel0->setAlignment(Qt::AlignCenter);
    actualTempLabel0->setStyleSheet(
        "color: #C62828; "
        "background: transparent; "
        "padding: 8px; "
        "border-radius: 8px;"
    );

    heatTempLabel = new QLabel("25°C");
    heatTempLabel->setFont(tempFont);
    heatTempLabel->setAlignment(Qt::AlignCenter);
    heatTempLabel->setFixedWidth(220);  // 固定宽度
    heatTempLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    heatTempLabel->setStyleSheet(
        "color: #B71C1C; "
        "background: transparent; "
        "padding: 10px 20px; "
        "border-radius: 12px; "
        "border: 2px dashed #D32F2F;"
    );
    heatTempLabel->setCursor(Qt::PointingHandCursor);
    heatTempLabel->installEventFilter(this);

    heatSlider = new QSlider(Qt::Vertical);
    heatSlider->setMinimum(20);
    heatSlider->setMaximum(55);
    heatSlider->setValue(25);
    heatSlider->setMinimumHeight(250);
    heatSlider->setMinimumWidth(60);  // 增加宽度方便拖动
    heatSlider->setStyleSheet(
        "QSlider { background: transparent; }"
        "QSlider::groove:vertical {"
        "    background: rgba(255, 255, 255, 0.4);"
        "    width: 30px;"
        "    border-radius: 15px;"
        "    border: 2px solid rgba(211, 47, 47, 0.5);"
        "}"
        "QSlider::handle:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 #F44336, stop:1 #D32F2F);"
        "    height: 50px;"
        "    width: 50px;"
        "    margin: -10px -10px;"
        "    border-radius: 25px;"
        "    border: 3px solid white;"
        "    box-shadow: 0 2px 5px rgba(0,0,0,0.3);"
        "}"
        "QSlider::handle:vertical:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 #E53935, stop:1 #C62828);"
        "    border: 3px solid #FFEBEE;"
        "}"
        "QSlider::handle:vertical:pressed {"
        "    background: #B71C1C;"
        "}"
    );

    heatStartBtn = new QPushButton("开启制热");
    heatStartBtn->setMinimumSize(180, 70);
    heatStartBtn->setCheckable(true);
    heatStartBtn->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #666; font-size: 22px; border-radius: 10px; }"
        "QPushButton:pressed { background-color: #BDBDBD; }"
        "QPushButton:checked { background-color: #D32F2F; color: white; }"
        "QPushButton:disabled { background-color: #F5F5F5; color: #BDBDBD; }"
    );
    
    heatStopBtn = new QPushButton("停止制热");
    heatStopBtn->setMinimumSize(180, 70);
    heatStopBtn->setEnabled(false);
    heatStopBtn->setStyleSheet(
        "QPushButton { background-color: #FF5722; color: white; font-size: 22px; border-radius: 10px; }"
        "QPushButton:pressed { background-color: #E64A19; }"
        "QPushButton:disabled { background-color: #F5F5F5; color: #BDBDBD; }"
    );
    
    QHBoxLayout *heatBtnLayout = new QHBoxLayout();
    heatBtnLayout->setSpacing(10);
    heatBtnLayout->addWidget(heatStartBtn);
    heatBtnLayout->addWidget(heatStopBtn);

    heatLayout->addWidget(heatTitleLabel);
    heatLayout->addWidget(actualTempLabel0);
    heatLayout->addWidget(heatTempLabel, 0, Qt::AlignCenter);
    heatLayout->addWidget(heatSlider, 0, Qt::AlignCenter);
    heatLayout->addLayout(heatBtnLayout);

    mainLayout->addWidget(coolWidget);
    mainLayout->addWidget(heatWidget);

    // 连接信号槽
    connect(heatSlider, &QSlider::valueChanged, this, &TemperaturePage::onHeatSliderChanged);
    connect(coolSlider, &QSlider::valueChanged, this, &TemperaturePage::onCoolSliderChanged);
    connect(heatStartBtn, &QPushButton::clicked, this, &TemperaturePage::onHeatStartClicked);
    connect(heatStopBtn, &QPushButton::clicked, this, &TemperaturePage::onHeatStopClicked);
    connect(coolStartBtn, &QPushButton::clicked, this, &TemperaturePage::onCoolStartClicked);
    connect(coolStopBtn, &QPushButton::clicked, this, &TemperaturePage::onCoolStopClicked);
}

void TemperaturePage::onHeatSliderChanged(int value)
{
    heatTempLabel->setText(QString("%1°C").arg(value));
    updateHeatButtons();
}

void TemperaturePage::onCoolSliderChanged(int value)
{
    coolTempLabel->setText(QString("%1°C").arg(value));
    updateCoolButtons();
}

void TemperaturePage::onHeatStartClicked()
{
    if (!hasValidTemp0) return;
    
    int targetTemp = heatSlider->value();
    
    // 检查设定温度是否高于当前温度
    if (targetTemp <= lastValidTemp0) {
        heatStartBtn->setChecked(false);
        return;
    }
    
    heatEnabled = true;
    heatStartBtn->setChecked(true);
    heatStartBtn->setEnabled(false);  // 禁用按钮，不可再点击
    heatStartBtn->setText("加热中");
    heatStopBtn->setEnabled(true);
    
    // 立即执行温度控制
    controlHeating(lastValidTemp0);
}

void TemperaturePage::onHeatStopClicked()
{
    heatEnabled = false;
    heatStartBtn->setChecked(false);
    heatStartBtn->setEnabled(true);  // 重新启用按钮
    heatStopBtn->setEnabled(false);
    setHeatingPin(false);
    updateHeatButtons();
}

void TemperaturePage::onCoolStartClicked()
{
    if (!hasValidTemp1) return;
    
    int targetTemp = coolSlider->value();
    
    // 检查设定温度是否低于当前温度
    if (targetTemp >= lastValidTemp1) {
        coolStartBtn->setChecked(false);
        return;
    }
    
    coolEnabled = true;
    coolStartBtn->setChecked(true);
    coolStartBtn->setEnabled(false);  // 禁用按钮，不可再点击
    coolStartBtn->setText("制冷中");
    coolStopBtn->setEnabled(true);
    
    // 立即执行温度控制
    controlCooling(lastValidTemp1);
}

void TemperaturePage::onCoolStopClicked()
{
    coolEnabled = false;
    coolStartBtn->setChecked(false);
    coolStartBtn->setEnabled(true);  // 重新启用按钮
    coolStopBtn->setEnabled(false);
    setCoolingPin(false);
    updateCoolButtons();
}

// 更新制热按钮状态
void TemperaturePage::updateHeatButtons()
{
    if (!hasValidTemp0) {
        heatStartBtn->setEnabled(false);
        heatStartBtn->setText("等待温度");
        return;
    }
    
    int targetTemp = heatSlider->value();
    
    if (heatEnabled) {
        // 已开启制热，保持状态
        return;
    }
    
    // 检查设定温度是否高于当前温度
    if (targetTemp <= lastValidTemp0) {
        heatStartBtn->setEnabled(false);
        heatStartBtn->setText("设定温度较低");
    } else {
        heatStartBtn->setEnabled(true);
        heatStartBtn->setText("开启制热");
    }
}

// 更新制冷按钮状态
void TemperaturePage::updateCoolButtons()
{
    if (!hasValidTemp1) {
        coolStartBtn->setEnabled(false);
        coolStartBtn->setText("等待温度");
        return;
    }
    
    int targetTemp = coolSlider->value();
    
    if (coolEnabled) {
        // 已开启制冷，保持状态
        return;
    }
    
    // 检查设定温度是否低于当前温度
    if (targetTemp >= lastValidTemp1) {
        coolStartBtn->setEnabled(false);
        coolStartBtn->setText("设定温度较高");
    } else {
        coolStartBtn->setEnabled(true);
        coolStartBtn->setText("开启制冷");
    }
}

void TemperaturePage::onTemperatureUpdated(QString temp0, QString temp1)
{
    // 处理传感器0的温度（制热区域）
    QRegExp rx0("([0-9.]+)");
    if (rx0.indexIn(temp0) != -1) {
        float currentTemp0 = rx0.cap(1).toFloat();
        
        // 温度滤波：检查变化幅度
        if (hasValidTemp0) {
            float tempDiff = qAbs(currentTemp0 - lastValidTemp0);
            if (tempDiff > 10.0f) {
                // 温度变化超过10度，认为异常，保持上一次温度
                currentTemp0 = lastValidTemp0;
            } else {
                // 温度正常，更新上一次有效温度
                lastValidTemp0 = currentTemp0;
            }
        } else {
            // 第一次获取温度，直接使用
            lastValidTemp0 = currentTemp0;
            hasValidTemp0 = true;
        }
        
        actualTempLabel0->setText(QString("当前: %1℃").arg(lastValidTemp0, 0, 'f', 2));
        
        // 更新按钮状态
        updateHeatButtons();
        
        // 执行制热控制
        controlHeating(lastValidTemp0);
    } else {
        // 读取失败，保持上一次温度
        if (hasValidTemp0) {
            actualTempLabel0->setText(QString("当前: %1℃").arg(lastValidTemp0, 0, 'f', 2));
            controlHeating(lastValidTemp0);
        }
    }
    
    // 处理传感器1的温度（制冷区域）
    QRegExp rx1("([0-9.]+)");
    if (rx1.indexIn(temp1) != -1) {
        float currentTemp1 = rx1.cap(1).toFloat();
        
        // 温度滤波：检查变化幅度
        if (hasValidTemp1) {
            float tempDiff = qAbs(currentTemp1 - lastValidTemp1);
            if (tempDiff > 10.0f) {
                // 温度变化超过10度，认为异常，保持上一次温度
                currentTemp1 = lastValidTemp1;
            } else {
                // 温度正常，更新上一次有效温度
                lastValidTemp1 = currentTemp1;
            }
        } else {
            // 第一次获取温度，直接使用
            lastValidTemp1 = currentTemp1;
            hasValidTemp1 = true;
        }
        
        actualTempLabel1->setText(QString("当前: %1℃").arg(lastValidTemp1, 0, 'f', 2));
        
        // 更新按钮状态
        updateCoolButtons();
        
        // 执行制冷控制
        controlCooling(lastValidTemp1);
    } else {
        // 读取失败，保持上一次温度
        if (hasValidTemp1) {
            actualTempLabel1->setText(QString("当前: %1℃").arg(lastValidTemp1, 0, 'f', 2));
            controlCooling(lastValidTemp1);
        }
    }
}

bool TemperaturePage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == heatTempLabel) {
            // 点击制热温度标签
            showHeatTempInput();
            return true;
        } else if (obj == coolTempLabel) {
            // 点击制冷温度标签
            showCoolTempInput();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TemperaturePage::showHeatTempInput()
{
    NumKeyboard keyboard("设置制热温度", heatSlider->value(), 
                        heatSlider->minimum(), heatSlider->maximum(), "°C", this);
    if (keyboard.exec() == QDialog::Accepted) {
        int newValue = keyboard.getValue();
        heatSlider->setValue(newValue);
    }
}

void TemperaturePage::showCoolTempInput()
{
    NumKeyboard keyboard("设置制冷温度", coolSlider->value(), 
                        coolSlider->minimum(), coolSlider->maximum(), "°C", this);
    if (keyboard.exec() == QDialog::Accepted) {
        int newValue = keyboard.getValue();
        coolSlider->setValue(newValue);
    }
}

// 控制加热引脚
void TemperaturePage::setHeatingPin(bool on)
{
    if (heatingPinState == on) return;  // 状态未变化，不重复操作
    
    QString cmd = QString("echo %1 > /sys/class/leds/gpio1b0/brightness").arg(on ? 1 : 0);
    
    // 使用异步方式执行命令，避免阻塞UI
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, &QProcess::deleteLater);
    process->start("sh", QStringList() << "-c" << cmd);
    
    heatingPinState = on;
    
    // 更新按钮显示（仅在加热中时更新文字）
    if (heatEnabled && !heatStartBtn->isEnabled()) {
        if (on) {
            heatStartBtn->setText("🔥 加热中");
        } else {
            heatStartBtn->setText("⏸ 待机中");
        }
    }
    
    qDebug() << "加热引脚:" << (on ? "开启" : "关闭");
}

// 控制制冷引脚
void TemperaturePage::setCoolingPin(bool on)
{
    if (coolingPinState == on) return;  // 状态未变化，不重复操作
    
    QString cmd = QString("echo %1 > /sys/class/leds/gpio1a4/brightness").arg(on ? 1 : 0);
    
    // 使用异步方式执行命令，避免阻塞UI
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, &QProcess::deleteLater);
    process->start("sh", QStringList() << "-c" << cmd);
    
    coolingPinState = on;
    
    // 更新按钮显示（仅在制冷中时更新文字）
    if (coolEnabled && !coolStartBtn->isEnabled()) {
        if (on) {
            coolStartBtn->setText("❄ 制冷中");
        } else {
            coolStartBtn->setText("⏸ 待机中");
        }
    }
    
    qDebug() << "制冷引脚:" << (on ? "开启" : "关闭");
}

// 制热控制逻辑
void TemperaturePage::controlHeating(float currentTemp)
{
    if (!heatEnabled) {
        setHeatingPin(false);
        return;
    }
    
    int targetTemp = heatSlider->value();
    
    // 如果当前正在加热
    if (heatingPinState) {
        // 达到或超过目标温度，关闭加热
        if (currentTemp >= targetTemp) {
            setHeatingPin(false);
        }
    } else {
        // 如果当前未加热，温度低于目标温度-2度时开启加热
        if (currentTemp <= targetTemp - 2.0f) {
            setHeatingPin(true);
        }
    }
}

// 制冷控制逻辑
void TemperaturePage::controlCooling(float currentTemp)
{
    if (!coolEnabled) {
        setCoolingPin(false);
        return;
    }
    
    int targetTemp = coolSlider->value();
    
    // 如果当前正在制冷
    if (coolingPinState) {
        // 达到或低于目标温度，关闭制冷
        if (currentTemp <= targetTemp) {
            setCoolingPin(false);
        }
    } else {
        // 如果当前未制冷，温度高于目标温度+2度时开启制冷
        if (currentTemp >= targetTemp + 2.0f) {
            setCoolingPin(true);
        }
    }
}
