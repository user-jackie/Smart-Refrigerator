#include "sterilizationpage.h"
#include "devicecontroller.h"
#include "numkeyboard.h"
#include <QFont>
#include <QHBoxLayout>
#include <QProcess>
#include <QDebug>
#include <QCoreApplication>

SterilizationPage::SterilizationPage(QWidget *parent)
    : QWidget(parent), uvEnabled(false), airEnabled(false), 
      uvRemainingTime(0), airRemainingTime(0), uvTimeValue(30), airTimeValue(60)
{
    setupUI();
    
    // 初始化GPIO引脚为低电平
    setGPIO("/sys/class/leds/gpio1b2/brightness", false);  // 杀菌引脚
    setGPIO("/sys/class/leds/gpio1b1/brightness", false);  // 净化引脚
    
    uvTimer = new QTimer(this);
    connect(uvTimer, &QTimer::timeout, this, &SterilizationPage::updateUVTimer);
    
    airTimer = new QTimer(this);
    connect(airTimer, &QTimer::timeout, this, &SterilizationPage::updateAirTimer);
    
    // 连接设备控制器信号
    DeviceController *controller = DeviceController::instance();
    connect(controller, &DeviceController::sterilizationStart, this, [this](int duration) {
        qDebug() << "收到杀菌开始指令，时长:" << duration << "分钟";
        if (uvEnabled) {
            // 已经在运行，返回剩余时间
            int minutes = uvRemainingTime / 60;
            int seconds = uvRemainingTime % 60;
            QString status;
            if (seconds > 0) {
                status = QString("正在杀菌，杀菌剩余时间%1分钟%2秒").arg(minutes).arg(seconds);
            } else {
                status = QString("正在杀菌，杀菌剩余时间%1分钟").arg(minutes);
            }
            DeviceController::instance()->sendStatusToAI(status);
        } else {
            // 未运行，开始杀菌
            // 先更新时间值和显示
            uvTimeValue = duration;
            uvTimeLabel->setText(QString::number(uvTimeValue) + " min");
            // 强制刷新界面，确保显示更新
            uvTimeLabel->update();
            QCoreApplication::processEvents();
            // 然后开始杀菌
            onUVSwitchClicked();
        }
    });
    connect(controller, &DeviceController::sterilizationStop, this, [this]() {
        qDebug() << "收到杀菌停止指令";
        if (uvEnabled) {
            onUVSwitchClicked();
        }
    });
    connect(controller, &DeviceController::sterilizationSetTime, this, [this](int duration) {
        qDebug() << "收到杀菌设置时间指令:" << duration << "分钟";
        uvTimeValue = duration;
        uvTimeLabel->setText(QString::number(uvTimeValue) + " min");
        
        // 如果正在运行，也更新剩余时间
        if (uvEnabled) {
            uvRemainingTime = duration * 60;
            qDebug() << "杀菌正在运行，已更新剩余时间为" << duration << "分钟";
            // 发送确认消息
            DeviceController::instance()->sendStatusToAI(
                QString("已将杀菌时间修改为%1分钟").arg(duration)
            );
        }
    });
    connect(controller, &DeviceController::sterilizationQuery, this, [this]() {
        qDebug() << "收到杀菌查询指令";
        QString status;
        if (uvEnabled) {
            int minutes = uvRemainingTime / 60;
            int seconds = uvRemainingTime % 60;
            status = QString("杀菌正在运行，剩余%1分钟%2秒").arg(minutes).arg(seconds);
        } else {
            status = "杀菌未运行";
        }
        DeviceController::instance()->sendStatusToAI(status);
    });
    
    connect(controller, &DeviceController::purificationStart, this, [this](int duration) {
        qDebug() << "收到净化开始指令，时长:" << duration << "分钟";
        if (airEnabled) {
            // 已经在运行，返回剩余时间
            int minutes = airRemainingTime / 60;
            int seconds = airRemainingTime % 60;
            QString status;
            if (seconds > 0) {
                status = QString("正在净化，空气净化剩余时间%1分钟%2秒").arg(minutes).arg(seconds);
            } else {
                status = QString("正在净化，空气净化剩余时间%1分钟").arg(minutes);
            }
            DeviceController::instance()->sendStatusToAI(status);
        } else {
            // 未运行，开始净化
            // 先更新时间值和显示
            airTimeValue = duration;
            airTimeLabel->setText(QString::number(airTimeValue) + " min");
            // 强制刷新界面，确保显示更新
            airTimeLabel->update();
            QCoreApplication::processEvents();
            // 然后开始净化
            onAirSwitchClicked();
        }
    });
    connect(controller, &DeviceController::purificationStop, this, [this]() {
        qDebug() << "收到净化停止指令";
        if (airEnabled) {
            onAirSwitchClicked();
        }
    });
    connect(controller, &DeviceController::purificationSetTime, this, [this](int duration) {
        qDebug() << "收到净化设置时间指令:" << duration << "分钟";
        airTimeValue = duration;
        airTimeLabel->setText(QString::number(airTimeValue) + " min");
        
        // 如果正在运行，也更新剩余时间
        if (airEnabled) {
            airRemainingTime = duration * 60;
            qDebug() << "空气净化正在运行，已更新剩余时间为" << duration << "分钟";
            // 发送确认消息
            DeviceController::instance()->sendStatusToAI(
                QString("已将空气净化时间修改为%1分钟").arg(duration)
            );
        }
    });
    connect(controller, &DeviceController::purificationQuery, this, [this]() {
        qDebug() << "收到净化查询指令";
        QString status;
        if (airEnabled) {
            int minutes = airRemainingTime / 60;
            int seconds = airRemainingTime % 60;
            status = QString("空气净化正在运行，剩余%1分钟%2秒").arg(minutes).arg(seconds);
        } else {
            status = "空气净化未运行";
        }
        DeviceController::instance()->sendStatusToAI(status);
    });
}

void SterilizationPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 设置渐变背景
    this->setStyleSheet(
        "SterilizationPage { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #E8EAF6, stop:0.5 #E1F5FE, stop:1 #F3E5F5); "
        "}"
    );

    // 紫外线杀菌区域
    QWidget *uvWidget = new QWidget();
    uvWidget->setStyleSheet(
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #FFFFFF, stop:1 #F3E5F5); "
        "border-radius: 15px; "
        "}"
    );
    uvWidget->setGraphicsEffect(createShadowEffect());
    
    QVBoxLayout *uvLayout = new QVBoxLayout(uvWidget);
    uvLayout->setSpacing(15);
    uvLayout->setContentsMargins(25, 20, 25, 20);

    uvTitleLabel = new QLabel("紫外线杀菌");
    QFont titleFont;
    titleFont.setPointSize(26);
    titleFont.setBold(true);
    uvTitleLabel->setFont(titleFont);
    uvTitleLabel->setAlignment(Qt::AlignCenter);
    uvTitleLabel->setStyleSheet("color: #9C27B0; background: transparent;");

    QHBoxLayout *uvControlLayout = new QHBoxLayout();
    
    QLabel *uvTimeLabelText = new QLabel("定时：");
    QFont labelFont;
    labelFont.setPointSize(22);
    uvTimeLabelText->setFont(labelFont);
    uvTimeLabelText->setStyleSheet("background: transparent;");

    uvTimeLabel = new ClickableLabel();
    uvTimeLabel->setText(QString::number(uvTimeValue) + " min");
    uvTimeLabel->setFont(labelFont);
    uvTimeLabel->setMinimumSize(100, 40);
    uvTimeLabel->setAlignment(Qt::AlignCenter);
    uvTimeLabel->setStyleSheet(
        "ClickableLabel { "
        "background: transparent; "
        "color: #9C27B0; "
        "font-weight: bold; "
        "text-decoration: underline; "
        "}"
    );
    uvTimeLabel->setCursor(Qt::PointingHandCursor);
    connect(uvTimeLabel, &ClickableLabel::clicked, this, &SterilizationPage::onUVTimeClicked);

    uvControlLayout->addWidget(uvTimeLabelText);
    uvControlLayout->addWidget(uvTimeLabel);
    uvControlLayout->addStretch();

    uvSwitchBtn = new QPushButton("开启杀菌");
    uvSwitchBtn->setMinimumSize(180, 55);
    uvSwitchBtn->setStyleSheet(
        "QPushButton { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F5F5F5, stop:1 #E0E0E0); "
        "color: #666; "
        "font-size: 24px; "
        "font-weight: bold; "
        "border-radius: 10px; "
        "border: 2px solid #BDBDBD; "
        "}"
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #BDBDBD); "
        "}"
    );

    uvStatusLabel = new QLabel("状态：待机中");
    uvStatusLabel->setFont(labelFont);
    uvStatusLabel->setAlignment(Qt::AlignCenter);
    uvStatusLabel->setStyleSheet("color: #666; background: transparent;");

    uvLayout->addWidget(uvTitleLabel);
    uvLayout->addLayout(uvControlLayout);
    uvLayout->addWidget(uvSwitchBtn, 0, Qt::AlignCenter);
    uvLayout->addWidget(uvStatusLabel);

    // 空气净化区域
    QWidget *airWidget = new QWidget();
    airWidget->setStyleSheet(
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #FFFFFF, stop:1 #E0F7FA); "
        "border-radius: 15px; "
        "}"
    );
    airWidget->setGraphicsEffect(createShadowEffect());
    
    QVBoxLayout *airLayout = new QVBoxLayout(airWidget);
    airLayout->setSpacing(15);
    airLayout->setContentsMargins(25, 20, 25, 20);

    airTitleLabel = new QLabel("空气净化");
    airTitleLabel->setFont(titleFont);
    airTitleLabel->setAlignment(Qt::AlignCenter);
    airTitleLabel->setStyleSheet("color: #00BCD4; background: transparent;");

    QHBoxLayout *airControlLayout = new QHBoxLayout();
    
    QLabel *airTimeLabelText = new QLabel("定时：");
    airTimeLabelText->setFont(labelFont);
    airTimeLabelText->setStyleSheet("background: transparent;");

    airTimeLabel = new ClickableLabel();
    airTimeLabel->setText(QString::number(airTimeValue) + " min");
    airTimeLabel->setFont(labelFont);
    airTimeLabel->setMinimumSize(100, 40);
    airTimeLabel->setAlignment(Qt::AlignCenter);
    airTimeLabel->setStyleSheet(
        "ClickableLabel { "
        "background: transparent; "
        "color: #00BCD4; "
        "font-weight: bold; "
        "text-decoration: underline; "
        "}"
    );
    airTimeLabel->setCursor(Qt::PointingHandCursor);
    connect(airTimeLabel, &ClickableLabel::clicked, this, &SterilizationPage::onAirTimeClicked);

    airControlLayout->addWidget(airTimeLabelText);
    airControlLayout->addWidget(airTimeLabel);
    airControlLayout->addStretch();

    airSwitchBtn = new QPushButton("开启净化");
    airSwitchBtn->setMinimumSize(180, 55);
    airSwitchBtn->setStyleSheet(
        "QPushButton { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F5F5F5, stop:1 #E0E0E0); "
        "color: #666; "
        "font-size: 24px; "
        "font-weight: bold; "
        "border-radius: 10px; "
        "border: 2px solid #BDBDBD; "
        "}"
        "QPushButton:pressed { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #BDBDBD); "
        "}"
    );

    airStatusLabel = new QLabel("状态：待机中");
    airStatusLabel->setFont(labelFont);
    airStatusLabel->setAlignment(Qt::AlignCenter);
    airStatusLabel->setStyleSheet("color: #666; background: transparent;");

    airLayout->addWidget(airTitleLabel);
    airLayout->addLayout(airControlLayout);
    airLayout->addWidget(airSwitchBtn, 0, Qt::AlignCenter);
    airLayout->addWidget(airStatusLabel);

    mainLayout->addWidget(uvWidget);
    mainLayout->addWidget(airWidget);

    // 连接信号槽
    connect(uvSwitchBtn, &QPushButton::clicked, this, &SterilizationPage::onUVSwitchClicked);
    connect(airSwitchBtn, &QPushButton::clicked, this, &SterilizationPage::onAirSwitchClicked);
}

void SterilizationPage::onUVSwitchClicked()
{
    uvEnabled = !uvEnabled;
    if (uvEnabled) {
        // 确保使用当前的 uvTimeValue 计算剩余时间
        uvRemainingTime = uvTimeValue * 60;
        // 确保显示标签显示正确的时间值
        uvTimeLabel->setText(QString::number(uvTimeValue) + " min");
        
        uvSwitchBtn->setText("停止杀菌");
        uvSwitchBtn->setStyleSheet(
            "QPushButton { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #BA68C8, stop:1 #9C27B0); "
            "color: white; "
            "font-size: 24px; "
            "font-weight: bold; "
            "border-radius: 10px; "
            "border: none; "
            "}"
            "QPushButton:pressed { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #9C27B0, stop:1 #7B1FA2); "
            "}"
        );
        uvTimeLabel->setEnabled(false);
        uvTimeLabel->setStyleSheet(
            "ClickableLabel { "
            "background: transparent; "
            "color: #999; "
            "font-weight: bold; "
            "text-decoration: none; "
            "}"
        );
        
        setGPIO("/sys/class/leds/gpio1b2/brightness", true);
        
        uvTimer->start(1000);
        updateUVTimer();
    } else {
        uvSwitchBtn->setText("开启杀菌");
        uvSwitchBtn->setStyleSheet(
            "QPushButton { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F5F5F5, stop:1 #E0E0E0); "
            "color: #666; "
            "font-size: 24px; "
            "font-weight: bold; "
            "border-radius: 10px; "
            "border: 2px solid #BDBDBD; "
            "}"
            "QPushButton:pressed { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #BDBDBD); "
            "}"
        );
        uvStatusLabel->setText("状态：待机中");
        uvTimeLabel->setEnabled(true);
        uvTimeLabel->setStyleSheet(
            "ClickableLabel { "
            "background: transparent; "
            "color: #9C27B0; "
            "font-weight: bold; "
            "text-decoration: underline; "
            "}"
        );
        
        setGPIO("/sys/class/leds/gpio1b2/brightness", false);
        
        uvTimer->stop();
    }
}

void SterilizationPage::onUVTimeClicked()
{
    if (!uvEnabled) {
        NumKeyboard keyboard("紫外线杀菌定时", uvTimeValue, 1, 120, "分钟", this);
        if (keyboard.exec() == QDialog::Accepted) {
            uvTimeValue = keyboard.getValue();
            uvTimeLabel->setText(QString::number(uvTimeValue) + " min");
        }
    }
}

void SterilizationPage::onAirSwitchClicked()
{
    airEnabled = !airEnabled;
    if (airEnabled) {
        // 确保使用当前的 airTimeValue 计算剩余时间
        airRemainingTime = airTimeValue * 60;
        // 确保显示标签显示正确的时间值
        airTimeLabel->setText(QString::number(airTimeValue) + " min");
        
        airSwitchBtn->setText("停止净化");
        airSwitchBtn->setStyleSheet(
            "QPushButton { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4DD0E1, stop:1 #00BCD4); "
            "color: white; "
            "font-size: 24px; "
            "font-weight: bold; "
            "border-radius: 10px; "
            "border: none; "
            "}"
            "QPushButton:pressed { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00BCD4, stop:1 #0097A7); "
            "}"
        );
        airTimeLabel->setEnabled(false);
        airTimeLabel->setStyleSheet(
            "ClickableLabel { "
            "background: transparent; "
            "color: #999; "
            "font-weight: bold; "
            "text-decoration: none; "
            "}"
        );
        
        setGPIO("/sys/class/leds/gpio1b1/brightness", true);
        
        airTimer->start(1000);
        updateAirTimer();
    } else {
        airSwitchBtn->setText("开启净化");
        airSwitchBtn->setStyleSheet(
            "QPushButton { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F5F5F5, stop:1 #E0E0E0); "
            "color: #666; "
            "font-size: 24px; "
            "font-weight: bold; "
            "border-radius: 10px; "
            "border: 2px solid #BDBDBD; "
            "}"
            "QPushButton:pressed { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #BDBDBD); "
            "}"
        );
        airStatusLabel->setText("状态：待机中");
        airTimeLabel->setEnabled(true);
        airTimeLabel->setStyleSheet(
            "ClickableLabel { "
            "background: transparent; "
            "color: #00BCD4; "
            "font-weight: bold; "
            "text-decoration: underline; "
            "}"
        );
        
        setGPIO("/sys/class/leds/gpio1b1/brightness", false);
        
        airTimer->stop();
    }
}

void SterilizationPage::onAirTimeClicked()
{
    if (!airEnabled) {
        NumKeyboard keyboard("空气净化定时", airTimeValue, 1, 120, "分钟", this);
        if (keyboard.exec() == QDialog::Accepted) {
            airTimeValue = keyboard.getValue();
            airTimeLabel->setText(QString::number(airTimeValue) + " min");
        }
    }
}

void SterilizationPage::updateUVTimer()
{
    if (uvRemainingTime > 0) {
        uvRemainingTime--;
        int minutes = uvRemainingTime / 60;
        int seconds = uvRemainingTime % 60;
        uvStatusLabel->setText(QString("运行中：剩余 %1:%2")
                              .arg(minutes, 2, 10, QChar('0'))
                              .arg(seconds, 2, 10, QChar('0')));
    } else {
        uvEnabled = false;
        uvTimer->stop();
        
        setGPIO("/sys/class/leds/gpio1b2/brightness", false);
        
        uvSwitchBtn->setText("开启杀菌");
        uvSwitchBtn->setStyleSheet(
            "QPushButton { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F5F5F5, stop:1 #E0E0E0); "
            "color: #666; "
            "font-size: 24px; "
            "font-weight: bold; "
            "border-radius: 10px; "
            "border: 2px solid #BDBDBD; "
            "}"
            "QPushButton:pressed { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #BDBDBD); "
            "}"
        );
        uvStatusLabel->setText("状态：已完成");
        uvTimeLabel->setEnabled(true);
        uvTimeLabel->setStyleSheet(
            "ClickableLabel { "
            "background: transparent; "
            "color: #9C27B0; "
            "font-weight: bold; "
            "text-decoration: underline; "
            "}"
        );
        
        // 发送杀菌完成通知
        DeviceController::instance()->sendStatusToAI("杀菌完成");
    }
}

void SterilizationPage::updateAirTimer()
{
    if (airRemainingTime > 0) {
        airRemainingTime--;
        int minutes = airRemainingTime / 60;
        int seconds = airRemainingTime % 60;
        airStatusLabel->setText(QString("运行中：剩余 %1:%2")
                               .arg(minutes, 2, 10, QChar('0'))
                               .arg(seconds, 2, 10, QChar('0')));
    } else {
        airEnabled = false;
        airTimer->stop();
        
        setGPIO("/sys/class/leds/gpio1b1/brightness", false);
        
        airSwitchBtn->setText("开启净化");
        airSwitchBtn->setStyleSheet(
            "QPushButton { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F5F5F5, stop:1 #E0E0E0); "
            "color: #666; "
            "font-size: 24px; "
            "font-weight: bold; "
            "border-radius: 10px; "
            "border: 2px solid #BDBDBD; "
            "}"
            "QPushButton:pressed { "
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E0E0E0, stop:1 #BDBDBD); "
            "}"
        );
        airStatusLabel->setText("状态：已完成");
        airTimeLabel->setEnabled(true);
        airTimeLabel->setStyleSheet(
            "ClickableLabel { "
            "background: transparent; "
            "color: #00BCD4; "
            "font-weight: bold; "
            "text-decoration: underline; "
            "}"
        );
        
        // 发送空气净化完成通知
        DeviceController::instance()->sendStatusToAI("空气净化完成");
    }
}

void SterilizationPage::setGPIO(const QString &gpioPath, bool high)
{
    QProcess process;
    QString command = QString("echo %1 > %2").arg(high ? "1" : "0").arg(gpioPath);
    
    process.start("sh", QStringList() << "-c" << command);
    process.waitForFinished();
    
    if (process.exitCode() != 0) {
        qDebug() << "GPIO控制失败:" << gpioPath << "错误:" << process.readAllStandardError();
    } else {
        qDebug() << "GPIO控制成功:" << gpioPath << "=" << (high ? "HIGH" : "LOW");
    }
}

QGraphicsDropShadowEffect* SterilizationPage::createShadowEffect()
{
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 3);
    return shadow;
}
