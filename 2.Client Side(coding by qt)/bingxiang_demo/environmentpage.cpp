#include "environmentpage.h"
#include <QFont>
#include <QGridLayout>
#include <QDateTime>
#include <QMessageBox>
#include <QScrollArea>
#include <QMouseEvent>
#include <QDebug>

// ==================== EnvironmentPage ====================

EnvironmentPage::EnvironmentPage(QWidget *parent)
    : QWidget(parent)
    , tcpSocket(nullptr)
    , reconnectTimer(nullptr)
    , serverHost("127.0.0.1")
    , serverPort(8888)
    , isConnected(false)
{
    setupUI();
    setupTcpClient();
}

EnvironmentPage::~EnvironmentPage()
{
    if (tcpSocket) {
        tcpSocket->disconnectFromHost();
        tcpSocket->deleteLater();
    }
}

void EnvironmentPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 创建堆叠窗口
    stackedWidget = new QStackedWidget();
    
    // 创建主页面
    setupMainPage();
    
    // 创建详细页面
    weatherDetailPage = new WeatherDetailPage();
    forecastDetailPage = new ForecastDetailPage();
    
    // 添加到堆叠窗口
    stackedWidget->addWidget(mainPage);
    stackedWidget->addWidget(weatherDetailPage);
    stackedWidget->addWidget(forecastDetailPage);
    
    // 连接返回信号
    connect(weatherDetailPage, &WeatherDetailPage::backClicked, this, &EnvironmentPage::onBackToMain);
    connect(forecastDetailPage, &ForecastDetailPage::backClicked, this, &EnvironmentPage::onBackToMain);
    
    mainLayout->addWidget(stackedWidget);
}

void EnvironmentPage::setupMainPage()
{
    mainPage = new QWidget();
    mainPage->setStyleSheet("background-color: #E8F5E9;");
    
    QVBoxLayout *pageLayout = new QVBoxLayout(mainPage);
    pageLayout->setSpacing(20);
    pageLayout->setContentsMargins(30, 30, 30, 30);
    
    // 顶部区域：标题在左，控制选项在右
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(20);
    
    // 左侧：标题
    QLabel *titleLabel = new QLabel("环境监测");
    QFont titleFont;
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet("color: #2E7D32; background: transparent;");
    
    // 右侧：控制区域
    QFrame *controlFrame = new QFrame();
    controlFrame->setStyleSheet("QFrame { background-color: #E8F5E9; border-radius: 12px; padding: 12px; }");
    QHBoxLayout *controlLayout = new QHBoxLayout(controlFrame);
    controlLayout->setSpacing(12);
    controlLayout->setContentsMargins(15, 10, 15, 10);
    
    // 刷新按钮
    refreshButton = new QPushButton("🔄 刷新");
    refreshButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; "
        "border-radius: 8px; padding: 8px 16px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
    );
    refreshButton->setMinimumHeight(40);
    connect(refreshButton, &QPushButton::clicked, this, &EnvironmentPage::onRefreshClicked);
    
    // 城市选择
    QLabel *cityLabel = new QLabel("城市:");
    QFont labelFont;
    labelFont.setPointSize(14);
    cityLabel->setFont(labelFont);
    cityLabel->setStyleSheet("color: #333;");
    
    cityComboBox = new QComboBox();
    cityComboBox->addItem("北京");
    cityComboBox->addItem("郑州");
    cityComboBox->addItem("上饶");
    cityComboBox->setStyleSheet(
        "QComboBox { background-color: white; border: 2px solid #ddd; "
        "border-radius: 6px; padding: 6px 12px; font-size: 14px; min-width: 90px; }"
        "QComboBox:hover { border-color: #4CAF50; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: url(down_arrow.png); width: 10px; height: 10px; }"
    );
    cityComboBox->setMinimumHeight(40);
    connect(cityComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &EnvironmentPage::onCityChanged);
    
    // 更新间隔设置
    QLabel *intervalLabel = new QLabel("更新间隔:");
    intervalLabel->setFont(labelFont);
    intervalLabel->setStyleSheet("color: #333;");
    
    intervalSpinBox = new QSpinBox();
    intervalSpinBox->setRange(1, 60);
    intervalSpinBox->setValue(60);
    intervalSpinBox->setSuffix(" 分钟");
    intervalSpinBox->setStyleSheet(
        "QSpinBox { background-color: white; border: 2px solid #ddd; "
        "border-radius: 6px; padding: 6px 10px; font-size: 14px; min-width: 90px; }"
        "QSpinBox:hover { border-color: #2196F3; }"
    );
    intervalSpinBox->setMinimumHeight(40);
    
    setIntervalButton = new QPushButton("设置");
    setIntervalButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; "
        "border-radius: 8px; padding: 8px 16px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0b7dda; }"
        "QPushButton:pressed { background-color: #0a6bc4; }"
    );
    setIntervalButton->setMinimumHeight(40);
    connect(setIntervalButton, &QPushButton::clicked, this, &EnvironmentPage::onSetIntervalClicked);
    
    controlLayout->addWidget(refreshButton);
    controlLayout->addWidget(cityLabel);
    controlLayout->addWidget(cityComboBox);
    controlLayout->addWidget(intervalLabel);
    controlLayout->addWidget(intervalSpinBox);
    controlLayout->addWidget(setIntervalButton);
    
    // 组装顶部布局
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    topLayout->addWidget(controlFrame);
    
    pageLayout->addLayout(topLayout);
    
    // 左右框架容器
    QHBoxLayout *framesLayout = new QHBoxLayout();
    framesLayout->setSpacing(20);
    
    // 左侧框架：实时天气 + 空气质量
    leftFrame = new QFrame();
    leftFrame->setStyleSheet(
        "QFrame { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #FFE082, stop:1 #FFB74D); "
        "border-radius: 20px; }"
        "QFrame:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #FFD54F, stop:1 #FFA726); }"
    );
    leftFrame->setCursor(Qt::PointingHandCursor);
    leftFrame->installEventFilter(this);
    
    QVBoxLayout *leftLayout = new QVBoxLayout(leftFrame);
    leftLayout->setSpacing(15);
    leftLayout->setContentsMargins(25, 25, 25, 25);
    
    leftTitleLabel = new QLabel("实时天气 & 空气质量");
    QFont frameFont;
    frameFont.setPointSize(24);
    frameFont.setBold(true);
    leftTitleLabel->setFont(frameFont);
    leftTitleLabel->setAlignment(Qt::AlignCenter);
    leftTitleLabel->setStyleSheet("color: #E65100; background: transparent;");
    
    tempSummaryLabel = new QLabel("温度: --°C");
    QFont summaryFont;
    summaryFont.setPointSize(28);
    summaryFont.setBold(true);
    tempSummaryLabel->setFont(summaryFont);
    tempSummaryLabel->setAlignment(Qt::AlignCenter);
    tempSummaryLabel->setStyleSheet("color: #BF360C; background: transparent;");
    
    weatherSummaryLabel = new QLabel("天气: --");
    QFont infoFont;
    infoFont.setPointSize(20);
    weatherSummaryLabel->setFont(infoFont);
    weatherSummaryLabel->setAlignment(Qt::AlignCenter);
    weatherSummaryLabel->setStyleSheet("color: #4E342E; background: transparent;");
    
    aqiSummaryLabel = new QLabel("空气质量: --");
    aqiSummaryLabel->setFont(infoFont);
    aqiSummaryLabel->setAlignment(Qt::AlignCenter);
    aqiSummaryLabel->setStyleSheet("color: #1B5E20; background: transparent;");
    
    QLabel *clickHintLeft = new QLabel("点击查看详情 >");
    QFont hintFont;
    hintFont.setPointSize(16);
    clickHintLeft->setFont(hintFont);
    clickHintLeft->setAlignment(Qt::AlignCenter);
    clickHintLeft->setStyleSheet("color: #6D4C41; background: transparent;");
    
    leftLayout->addWidget(leftTitleLabel);
    leftLayout->addStretch();
    leftLayout->addWidget(tempSummaryLabel);
    leftLayout->addWidget(weatherSummaryLabel);
    leftLayout->addWidget(aqiSummaryLabel);
    leftLayout->addStretch();
    leftLayout->addWidget(clickHintLeft);
    
    // 右侧框架：降水预报 + 3天预报
    rightFrame = new QFrame();
    rightFrame->setStyleSheet(
        "QFrame { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #81D4FA, stop:1 #4FC3F7); "
        "border-radius: 20px; }"
        "QFrame:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "stop:0 #4FC3F7, stop:1 #29B6F6); }"
    );
    rightFrame->setCursor(Qt::PointingHandCursor);
    rightFrame->installEventFilter(this);
    
    QVBoxLayout *rightLayout = new QVBoxLayout(rightFrame);
    rightLayout->setSpacing(15);
    rightLayout->setContentsMargins(25, 25, 25, 25);
    
    rightTitleLabel = new QLabel("降水预报 & 3天预报");
    rightTitleLabel->setFont(frameFont);
    rightTitleLabel->setAlignment(Qt::AlignCenter);
    rightTitleLabel->setStyleSheet("color: #01579B; background: transparent;");
    
    precipSummaryLabel = new QLabel("降水: --");
    precipSummaryLabel->setFont(infoFont);
    precipSummaryLabel->setAlignment(Qt::AlignCenter);
    precipSummaryLabel->setStyleSheet("color: #0D47A1; background: transparent;");
    precipSummaryLabel->setWordWrap(true);
    
    forecastSummaryWidget = new QWidget();
    forecastSummaryWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *forecastSummaryLayout = new QVBoxLayout(forecastSummaryWidget);
    forecastSummaryLayout->setSpacing(8);
    forecastSummaryLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *clickHintRight = new QLabel("点击查看详情 >");
    clickHintRight->setFont(hintFont);
    clickHintRight->setAlignment(Qt::AlignCenter);
    clickHintRight->setStyleSheet("color: #004D40; background: transparent;");
    
    rightLayout->addWidget(rightTitleLabel);
    rightLayout->addWidget(precipSummaryLabel);
    rightLayout->addWidget(forecastSummaryWidget, 1);
    rightLayout->addWidget(clickHintRight);
    
    framesLayout->addWidget(leftFrame, 1);
    framesLayout->addWidget(rightFrame, 1);
    
    pageLayout->addLayout(framesLayout, 1);
}

bool EnvironmentPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == leftFrame) {
            onLeftFrameClicked();
            return true;
        } else if (obj == rightFrame) {
            onRightFrameClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void EnvironmentPage::setupTcpClient()
{
    tcpSocket = new QTcpSocket(this);
    
    connect(tcpSocket, &QTcpSocket::connected, this, &EnvironmentPage::onConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &EnvironmentPage::onDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &EnvironmentPage::onReadyRead);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &EnvironmentPage::onError);
    
    reconnectTimer = new QTimer(this);
    connect(reconnectTimer, &QTimer::timeout, this, &EnvironmentPage::connectToServer);
    
    connectToServer();
}

void EnvironmentPage::connectToServer()
{
    if (tcpSocket->state() == QAbstractSocket::UnconnectedState) {
        tcpSocket->connectToHost(serverHost, serverPort);
    }
}

void EnvironmentPage::onConnected()
{
    isConnected = true;
    reconnectTimer->stop();
    requestWeatherData();
}

void EnvironmentPage::onDisconnected()
{
    isConnected = false;
    reconnectTimer->start(5000);
}

void EnvironmentPage::onError(QAbstractSocket::SocketError error)
{
    if (!isConnected && !reconnectTimer->isActive()) {
        reconnectTimer->start(5000);
    }
}

void EnvironmentPage::onReadyRead()
{
    QByteArray data = tcpSocket->readAll();
    QString jsonStr = QString::fromUtf8(data);
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        weatherData = doc.object();
        parseWeatherData(weatherData);
    }
}

void EnvironmentPage::parseWeatherData(const QJsonObject &data)
{
    updateMainPageSummary();
    weatherDetailPage->updateData(data);
    forecastDetailPage->updateData(data);
}

void EnvironmentPage::updateMainPageSummary()
{
    // 更新左侧框架摘要
    if (weatherData.contains("current")) {
        QJsonObject current = weatherData["current"].toObject();
        tempSummaryLabel->setText("温度: " + current["temp"].toString() + "°C");
        weatherSummaryLabel->setText("天气: " + current["text"].toString());
    }
    
    if (weatherData.contains("air_quality")) {
        QJsonObject airQuality = weatherData["air_quality"].toObject();
        QString category = airQuality["category"].toString();
        int aqi = airQuality["aqi"].toInt();
        aqiSummaryLabel->setText(QString("空气质量: %1 (AQI %2)").arg(category).arg(aqi));
    }
    
    // 更新右侧框架摘要
    if (weatherData.contains("precipitation")) {
        QJsonObject precip = weatherData["precipitation"].toObject();
        precipSummaryLabel->setText("降水: " + precip["summary"].toString());
    }
    
    // 更新3天预报摘要
    if (weatherData.contains("forecast")) {
        QJsonArray forecast = weatherData["forecast"].toArray();
        
        QLayoutItem *item;
        while ((item = forecastSummaryWidget->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        
        for (int i = 0; i < qMin(3, forecast.size()); i++) {
            QJsonObject day = forecast[i].toObject();
            QLabel *dayLabel = new QLabel(QString("%1: %2 %3~%4°C")
                .arg(day["date"].toString().mid(5))
                .arg(day["textDay"].toString())
                .arg(day["tempMin"].toString())
                .arg(day["tempMax"].toString()));
            QFont dayFont;
            dayFont.setPointSize(16);
            dayLabel->setFont(dayFont);
            dayLabel->setStyleSheet("color: #004D40; background: transparent;");
            dayLabel->setAlignment(Qt::AlignCenter);
            forecastSummaryWidget->layout()->addWidget(dayLabel);
        }
    }
}

QString EnvironmentPage::formatDateTime(qint64 timestamp)
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    return dt.toString("yyyy-MM-dd hh:mm:ss");
}

void EnvironmentPage::requestWeatherData()
{
    if (isConnected) {
        QString request = "GET_WEATHER";
        tcpSocket->write(request.toUtf8());
        tcpSocket->flush();
    }
}

void EnvironmentPage::onLeftFrameClicked()
{
    weatherDetailPage->resetScrollPosition();
    stackedWidget->setCurrentWidget(weatherDetailPage);
}

void EnvironmentPage::onRightFrameClicked()
{
    forecastDetailPage->resetScrollPosition();
    stackedWidget->setCurrentWidget(forecastDetailPage);
}

void EnvironmentPage::onBackToMain()
{
    stackedWidget->setCurrentWidget(mainPage);
}

void EnvironmentPage::onRefreshClicked()
{
    requestWeatherData();
}

void EnvironmentPage::onCityChanged(int index)
{
    if (isConnected) {
        QString city = cityComboBox->currentText();
        QString request = QString("SET_CITY:%1").arg(city);
        tcpSocket->write(request.toUtf8());
        tcpSocket->flush();
        
        // 切换城市后立即刷新数据
        QTimer::singleShot(500, this, &EnvironmentPage::requestWeatherData);
    }
}

void EnvironmentPage::onSetIntervalClicked()
{
    if (isConnected) {
        int intervalMinutes = intervalSpinBox->value();
        int intervalSeconds = intervalMinutes * 60;
        QString request = QString("SET_INTERVAL:%1").arg(intervalSeconds);
        tcpSocket->write(request.toUtf8());
        tcpSocket->flush();
    }
}

// ==================== WeatherDetailPage ====================

WeatherDetailPage::WeatherDetailPage(QWidget *parent) 
    : QWidget(parent), isDragging(false)
{
    setupUI();
}

void WeatherDetailPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    this->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                       "stop:0 #FFF9C4, stop:1 #FFE082);");
    
    // 返回按钮
    backButton = new QPushButton("← 返回");
    backButton->setStyleSheet(
        "QPushButton { background-color: #FF6F00; color: white; "
        "border-radius: 10px; padding: 12px 30px; font-size: 20px; font-weight: bold; }"
        "QPushButton:hover { background-color: #E65100; }"
        "QPushButton:pressed { background-color: #BF360C; }"
    );
    backButton->setMaximumWidth(150);
    connect(backButton, &QPushButton::clicked, this, &WeatherDetailPage::backClicked);
    mainLayout->addWidget(backButton);
    
    // 标题栏
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("实时天气 & 空气质量详情");
    QFont titleFont;
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #E65100; background: transparent;");
    
    cityLabel = new QLabel("城市: --");
    QFont infoFont;
    infoFont.setPointSize(16);
    cityLabel->setFont(infoFont);
    cityLabel->setStyleSheet("color: #4E342E; background: transparent;");
    
    updateTimeLabel = new QLabel("更新: --");
    updateTimeLabel->setFont(infoFont);
    updateTimeLabel->setStyleSheet("color: #4E342E; background: transparent;");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(cityLabel);
    headerLayout->addWidget(updateTimeLabel);
    mainLayout->addLayout(headerLayout);
    
    // 滚动区域
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: #E0E0E0; width: 12px; border-radius: 6px; }"
        "QScrollBar::handle:vertical { background: #9E9E9E; border-radius: 6px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #757575; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    );
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFocusPolicy(Qt::StrongFocus);
    
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(20);
    
    // 实时天气卡片
    QFrame *weatherCard = new QFrame();
    weatherCard->setStyleSheet("QFrame { background-color: white; border-radius: 15px; padding: 20px; }");
    QGridLayout *weatherLayout = new QGridLayout(weatherCard);
    weatherLayout->setSpacing(15);
    
    QLabel *weatherTitle = new QLabel("🌡️ 实时天气");
    QFont sectionFont;
    sectionFont.setPointSize(22);
    sectionFont.setBold(true);
    weatherTitle->setFont(sectionFont);
    weatherTitle->setStyleSheet("color: #FF6F00;");
    weatherLayout->addWidget(weatherTitle, 0, 0, 1, 4);
    
    QFont labelFont;
    labelFont.setPointSize(16);
    QFont valueFont;
    valueFont.setPointSize(20);
    valueFont.setBold(true);
    
    int row = 1;
    
    QLabel *tempLabel = new QLabel("温度:");
    tempLabel->setFont(labelFont);
    tempValueLabel = new QLabel("--°C");
    tempValueLabel->setFont(valueFont);
    tempValueLabel->setStyleSheet("color: #FF5722;");
    weatherLayout->addWidget(tempLabel, row, 0);
    weatherLayout->addWidget(tempValueLabel, row, 1);
    
    QLabel *feelsLikeLabel = new QLabel("体感温度:");
    feelsLikeLabel->setFont(labelFont);
    feelsLikeValueLabel = new QLabel("--°C");
    feelsLikeValueLabel->setFont(valueFont);
    feelsLikeValueLabel->setStyleSheet("color: #FF9800;");
    weatherLayout->addWidget(feelsLikeLabel, row, 2);
    weatherLayout->addWidget(feelsLikeValueLabel, row, 3);
    row++;
    
    QLabel *weatherLabel = new QLabel("天气状况:");
    weatherLabel->setFont(labelFont);
    weatherTextLabel = new QLabel("--");
    weatherTextLabel->setFont(valueFont);
    weatherTextLabel->setStyleSheet("color: #2196F3;");
    weatherLayout->addWidget(weatherLabel, row, 0);
    weatherLayout->addWidget(weatherTextLabel, row, 1);
    
    QLabel *humidityLabel = new QLabel("湿度:");
    humidityLabel->setFont(labelFont);
    humidityValueLabel = new QLabel("--%");
    humidityValueLabel->setFont(valueFont);
    humidityValueLabel->setStyleSheet("color: #00BCD4;");
    weatherLayout->addWidget(humidityLabel, row, 2);
    weatherLayout->addWidget(humidityValueLabel, row, 3);
    row++;
    
    QLabel *windLabel = new QLabel("风力:");
    windLabel->setFont(labelFont);
    windValueLabel = new QLabel("--");
    windValueLabel->setFont(valueFont);
    windValueLabel->setStyleSheet("color: #4CAF50;");
    weatherLayout->addWidget(windLabel, row, 0);
    weatherLayout->addWidget(windValueLabel, row, 1);
    
    QLabel *pressureLabel = new QLabel("气压:");
    pressureLabel->setFont(labelFont);
    pressureValueLabel = new QLabel("-- hPa");
    pressureValueLabel->setFont(valueFont);
    pressureValueLabel->setStyleSheet("color: #9C27B0;");
    weatherLayout->addWidget(pressureLabel, row, 2);
    weatherLayout->addWidget(pressureValueLabel, row, 3);
    row++;
    
    QLabel *visibilityLabel = new QLabel("能见度:");
    visibilityLabel->setFont(labelFont);
    visibilityValueLabel = new QLabel("-- km");
    visibilityValueLabel->setFont(valueFont);
    visibilityValueLabel->setStyleSheet("color: #607D8B;");
    weatherLayout->addWidget(visibilityLabel, row, 0);
    weatherLayout->addWidget(visibilityValueLabel, row, 1);
    
    contentLayout->addWidget(weatherCard);
    
    // 空气质量卡片
    QFrame *airCard = new QFrame();
    airCard->setStyleSheet("QFrame { background-color: white; border-radius: 15px; padding: 20px; }");
    QGridLayout *airLayout = new QGridLayout(airCard);
    airLayout->setSpacing(15);
    
    QLabel *airTitle = new QLabel("🌿 空气质量详情");
    airTitle->setFont(sectionFont);
    airTitle->setStyleSheet("color: #4CAF50;");
    airLayout->addWidget(airTitle, 0, 0, 1, 4);
    
    row = 1;
    
    QLabel *aqiLabel = new QLabel("AQI指数:");
    aqiLabel->setFont(labelFont);
    aqiValueLabel = new QLabel("--");
    aqiValueLabel->setFont(valueFont);
    aqiValueLabel->setStyleSheet("color: #4CAF50;");
    airLayout->addWidget(aqiLabel, row, 0);
    airLayout->addWidget(aqiValueLabel, row, 1);
    
    aqiCategoryLabel = new QLabel("--");
    QFont categoryFont;
    categoryFont.setPointSize(18);
    categoryFont.setBold(true);
    aqiCategoryLabel->setFont(categoryFont);
    aqiCategoryLabel->setStyleSheet("color: #666;");
    airLayout->addWidget(aqiCategoryLabel, row, 2, 1, 2);
    row++;
    
    QLabel *pm25Label = new QLabel("PM2.5:");
    pm25Label->setFont(labelFont);
    pm25ValueLabel = new QLabel("-- μg/m³");
    pm25ValueLabel->setFont(valueFont);
    pm25ValueLabel->setStyleSheet("color: #FF9800;");
    airLayout->addWidget(pm25Label, row, 0);
    airLayout->addWidget(pm25ValueLabel, row, 1);
    
    QLabel *pm10Label = new QLabel("PM10:");
    pm10Label->setFont(labelFont);
    pm10ValueLabel = new QLabel("-- μg/m³");
    pm10ValueLabel->setFont(valueFont);
    pm10ValueLabel->setStyleSheet("color: #FF5722;");
    airLayout->addWidget(pm10Label, row, 2);
    airLayout->addWidget(pm10ValueLabel, row, 3);
    row++;
    
    QLabel *no2Label = new QLabel("NO₂:");
    no2Label->setFont(labelFont);
    no2ValueLabel = new QLabel("-- μg/m³");
    no2ValueLabel->setFont(valueFont);
    no2ValueLabel->setStyleSheet("color: #9C27B0;");
    airLayout->addWidget(no2Label, row, 0);
    airLayout->addWidget(no2ValueLabel, row, 1);
    
    QLabel *so2Label = new QLabel("SO₂:");
    so2Label->setFont(labelFont);
    so2ValueLabel = new QLabel("-- μg/m³");
    so2ValueLabel->setFont(valueFont);
    so2ValueLabel->setStyleSheet("color: #E91E63;");
    airLayout->addWidget(so2Label, row, 2);
    airLayout->addWidget(so2ValueLabel, row, 3);
    row++;
    
    QLabel *coLabel = new QLabel("CO:");
    coLabel->setFont(labelFont);
    coValueLabel = new QLabel("-- mg/m³");
    coValueLabel->setFont(valueFont);
    coValueLabel->setStyleSheet("color: #795548;");
    airLayout->addWidget(coLabel, row, 0);
    airLayout->addWidget(coValueLabel, row, 1);
    
    QLabel *o3Label = new QLabel("O₃:");
    o3Label->setFont(labelFont);
    o3ValueLabel = new QLabel("-- μg/m³");
    o3ValueLabel->setFont(valueFont);
    o3ValueLabel->setStyleSheet("color: #00BCD4;");
    airLayout->addWidget(o3Label, row, 2);
    airLayout->addWidget(o3ValueLabel, row, 3);
    
    contentLayout->addWidget(airCard);
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void WeatherDetailPage::updateData(const QJsonObject &data)
{
    // 更新时间和城市
    if (data.contains("timestamp")) {
        qint64 timestamp = static_cast<qint64>(data["timestamp"].toDouble());
        updateTimeLabel->setText("更新: " + formatDateTime(timestamp));
    }
    
    if (data.contains("city")) {
        QJsonObject city = data["city"].toObject();
        cityLabel->setText("城市: " + city["name"].toString());
    }
    
    // 更新实时天气
    if (data.contains("current")) {
        QJsonObject current = data["current"].toObject();
        tempValueLabel->setText(current["temp"].toString() + "°C");
        feelsLikeValueLabel->setText(current["feelsLike"].toString() + "°C");
        weatherTextLabel->setText(current["text"].toString());
        humidityValueLabel->setText(current["humidity"].toString() + "%");
        windValueLabel->setText(current["windDir"].toString() + " " + 
                               current["windScale"].toString() + "级");
        pressureValueLabel->setText(current["pressure"].toString() + " hPa");
        visibilityValueLabel->setText(current["vis"].toString() + " km");
    }
    
    // 更新空气质量
    if (data.contains("air_quality")) {
        QJsonObject airQuality = data["air_quality"].toObject();
        int aqi = airQuality["aqi"].toInt();
        aqiValueLabel->setText(QString::number(aqi));
        aqiCategoryLabel->setText(airQuality["category"].toString());
        
        // 根据AQI设置颜色
        if (aqi <= 50) {
            aqiValueLabel->setStyleSheet("color: #4CAF50;");
        } else if (aqi <= 100) {
            aqiValueLabel->setStyleSheet("color: #FF9800;");
        } else if (aqi <= 150) {
            aqiValueLabel->setStyleSheet("color: #FF5722;");
        } else {
            aqiValueLabel->setStyleSheet("color: #F44336;");
        }
        
        // 更新污染物
        if (airQuality.contains("pollutants")) {
            QJsonObject pollutants = airQuality["pollutants"].toObject();
            
            if (pollutants.contains("pm2p5")) {
                QJsonObject pm25 = pollutants["pm2p5"].toObject();
                pm25ValueLabel->setText(QString::number(pm25["value"].toDouble(), 'f', 1) + " μg/m³");
            }
            if (pollutants.contains("pm10")) {
                QJsonObject pm10 = pollutants["pm10"].toObject();
                pm10ValueLabel->setText(QString::number(pm10["value"].toDouble(), 'f', 1) + " μg/m³");
            }
            if (pollutants.contains("no2")) {
                QJsonObject no2 = pollutants["no2"].toObject();
                no2ValueLabel->setText(QString::number(no2["value"].toDouble(), 'f', 1) + " μg/m³");
            }
            if (pollutants.contains("so2")) {
                QJsonObject so2 = pollutants["so2"].toObject();
                so2ValueLabel->setText(QString::number(so2["value"].toDouble(), 'f', 1) + " μg/m³");
            }
            if (pollutants.contains("co")) {
                QJsonObject co = pollutants["co"].toObject();
                coValueLabel->setText(QString::number(co["value"].toDouble(), 'f', 2) + " mg/m³");
            }
            if (pollutants.contains("o3")) {
                QJsonObject o3 = pollutants["o3"].toObject();
                o3ValueLabel->setText(QString::number(o3["value"].toDouble(), 'f', 1) + " μg/m³");
            }
        }
    }
}

QString WeatherDetailPage::formatDateTime(qint64 timestamp)
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    return dt.toString("MM-dd hh:mm");
}

void WeatherDetailPage::wheelEvent(QWheelEvent *event)
{
    // qDebug() << "[WeatherDetailPage] wheelEvent triggered, delta:" << event->angleDelta().y();
    
    // 将鼠标滚轮事件转发给滚动区域的垂直滚动条
    if (scrollArea && scrollArea->verticalScrollBar()) {
        QScrollBar *scrollBar = scrollArea->verticalScrollBar();
        // 使用更自然的滚动速度
        int delta = -event->angleDelta().y() / 2;
        int oldValue = scrollBar->value();
        scrollBar->setValue(oldValue + delta);
        // qDebug() << "[WeatherDetailPage] Scroll from" << oldValue << "to" << scrollBar->value();
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void WeatherDetailPage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPos = event->pos();
        isDragging = true;
        // qDebug() << "[WeatherDetailPage] Mouse press at:" << dragStartPos;
    }
    QWidget::mousePressEvent(event);
}

void WeatherDetailPage::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        int deltaY = event->pos().y() - dragStartPos.y();
        // qDebug() << "[WeatherDetailPage] Mouse move, deltaY:" << deltaY;
        
        // 如果是垂直拖动，滚动内容
        if (qAbs(deltaY) > 5 && scrollArea && scrollArea->verticalScrollBar()) {
            QScrollBar *scrollBar = scrollArea->verticalScrollBar();
            int oldValue = scrollBar->value();
            scrollBar->setValue(oldValue - deltaY);
            dragStartPos = event->pos();
            // qDebug() << "[WeatherDetailPage] Scrolling from" << oldValue << "to" << scrollBar->value();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void WeatherDetailPage::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        int deltaY = event->pos().y() - dragStartPos.y();
        // qDebug() << "[WeatherDetailPage] Mouse release, total deltaY:" << deltaY;
        isDragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void WeatherDetailPage::resetScrollPosition()
{
    if (scrollArea && scrollArea->verticalScrollBar()) {
        scrollArea->verticalScrollBar()->setValue(0);
    }
}

// ==================== ForecastDetailPage ====================

ForecastDetailPage::ForecastDetailPage(QWidget *parent) 
    : QWidget(parent), isDragging(false)
{
    setupUI();
}

void ForecastDetailPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    this->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                       "stop:0 #B3E5FC, stop:1 #81D4FA);");
    
    // 返回按钮
    backButton = new QPushButton("← 返回");
    backButton->setStyleSheet(
        "QPushButton { background-color: #0277BD; color: white; "
        "border-radius: 10px; padding: 12px 30px; font-size: 20px; font-weight: bold; }"
        "QPushButton:hover { background-color: #01579B; }"
        "QPushButton:pressed { background-color: #004D40; }"
    );
    backButton->setMaximumWidth(150);
    connect(backButton, &QPushButton::clicked, this, &ForecastDetailPage::backClicked);
    mainLayout->addWidget(backButton);
    
    // 标题栏
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("降水预报 & 3天预报详情");
    QFont titleFont;
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #01579B; background: transparent;");
    
    cityLabel = new QLabel("城市: --");
    QFont infoFont;
    infoFont.setPointSize(16);
    cityLabel->setFont(infoFont);
    cityLabel->setStyleSheet("color: #004D40; background: transparent;");
    
    updateTimeLabel = new QLabel("更新: --");
    updateTimeLabel->setFont(infoFont);
    updateTimeLabel->setStyleSheet("color: #004D40; background: transparent;");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(cityLabel);
    headerLayout->addWidget(updateTimeLabel);
    mainLayout->addLayout(headerLayout);
    
    // 滚动区域
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: #E0E0E0; width: 12px; border-radius: 6px; }"
        "QScrollBar::handle:vertical { background: #9E9E9E; border-radius: 6px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #757575; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    );
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFocusPolicy(Qt::StrongFocus);
    
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(20);
    
    // 降水预报卡片
    QFrame *precipCard = new QFrame();
    precipCard->setStyleSheet("QFrame { background-color: white; border-radius: 15px; padding: 20px; }");
    QVBoxLayout *precipLayout = new QVBoxLayout(precipCard);
    precipLayout->setSpacing(15);
    
    QLabel *precipTitle = new QLabel("🌧️ 降水预报（未来2小时）");
    QFont sectionFont;
    sectionFont.setPointSize(22);
    sectionFont.setBold(true);
    precipTitle->setFont(sectionFont);
    precipTitle->setStyleSheet("color: #0277BD;");
    precipLayout->addWidget(precipTitle);
    
    precipSummaryLabel = new QLabel("--");
    QFont summaryFont;
    summaryFont.setPointSize(18);
    precipSummaryLabel->setFont(summaryFont);
    precipSummaryLabel->setStyleSheet("color: #01579B;");
    precipSummaryLabel->setWordWrap(true);
    precipLayout->addWidget(precipSummaryLabel);
    
    precipDetailWidget = new QWidget();
    precipDetailWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *precipDetailLayout = new QVBoxLayout(precipDetailWidget);
    precipDetailLayout->setSpacing(8);
    precipLayout->addWidget(precipDetailWidget);
    
    contentLayout->addWidget(precipCard);
    
    // 3天预报卡片
    QFrame *forecastCard = new QFrame();
    forecastCard->setStyleSheet("QFrame { background-color: white; border-radius: 15px; padding: 20px; }");
    QVBoxLayout *forecastLayout = new QVBoxLayout(forecastCard);
    forecastLayout->setSpacing(15);
    
    QLabel *forecastTitle = new QLabel("📅 3天天气预报");
    forecastTitle->setFont(sectionFont);
    forecastTitle->setStyleSheet("color: #FF6F00;");
    forecastLayout->addWidget(forecastTitle);
    
    forecastDetailWidget = new QWidget();
    forecastDetailWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *forecastDetailLayout = new QVBoxLayout(forecastDetailWidget);
    forecastDetailLayout->setSpacing(12);
    forecastLayout->addWidget(forecastDetailWidget);
    
    contentLayout->addWidget(forecastCard);
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void ForecastDetailPage::updateData(const QJsonObject &data)
{
    // 更新时间和城市
    if (data.contains("timestamp")) {
        qint64 timestamp = static_cast<qint64>(data["timestamp"].toDouble());
        updateTimeLabel->setText("更新: " + formatDateTime(timestamp));
    }
    
    if (data.contains("city")) {
        QJsonObject city = data["city"].toObject();
        cityLabel->setText("城市: " + city["name"].toString());
    }
    
    // 更新降水预报
    if (data.contains("precipitation")) {
        QJsonObject precip = data["precipitation"].toObject();
        precipSummaryLabel->setText(precip["summary"].toString());
        
        // 清空旧的详细信息
        QLayoutItem *item;
        while ((item = precipDetailWidget->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        
        // 添加分钟级降水详情
        if (precip.contains("minutely") && precip["minutely"].isArray()) {
            QJsonArray minutely = precip["minutely"].toArray();
            
            QFont detailFont;
            detailFont.setPointSize(14);
            
            for (int i = 0; i < qMin(12, minutely.size()); i += 2) {
                QJsonObject minute = minutely[i].toObject();
                QString text = QString("%1: %2mm")
                    .arg(minute["fxTime"].toString().mid(11, 5))
                    .arg(minute["precip"].toString());
                
                QLabel *label = new QLabel(text);
                label->setFont(detailFont);
                label->setStyleSheet("color: #0277BD; background: #E1F5FE; "
                                   "padding: 8px; border-radius: 5px;");
                precipDetailWidget->layout()->addWidget(label);
            }
        }
    }
    
    // 更新3天预报
    if (data.contains("forecast")) {
        QJsonArray forecast = data["forecast"].toArray();
        
        // 清空旧的预报
        QLayoutItem *item;
        while ((item = forecastDetailWidget->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        
        // 添加详细预报
        for (const QJsonValue &value : forecast) {
            QJsonObject day = value.toObject();
            
            QFrame *dayFrame = new QFrame();
            dayFrame->setStyleSheet("QFrame { background: #FFF3E0; border-radius: 10px; padding: 15px; }");
            QVBoxLayout *dayLayout = new QVBoxLayout(dayFrame);
            dayLayout->setSpacing(8);
            
            // 日期
            QLabel *dateLabel = new QLabel(day["date"].toString());
            QFont dateFont;
            dateFont.setPointSize(18);
            dateFont.setBold(true);
            dateLabel->setFont(dateFont);
            dateLabel->setStyleSheet("color: #E65100;");
            dayLayout->addWidget(dateLabel);
            
            // 天气和温度
            QHBoxLayout *mainInfoLayout = new QHBoxLayout();
            
            QLabel *dayWeatherLabel = new QLabel("白天: " + day["textDay"].toString());
            QFont infoFont;
            infoFont.setPointSize(16);
            dayWeatherLabel->setFont(infoFont);
            dayWeatherLabel->setStyleSheet("color: #FF6F00;");
            
            QLabel *nightWeatherLabel = new QLabel("夜间: " + day["textNight"].toString());
            nightWeatherLabel->setFont(infoFont);
            nightWeatherLabel->setStyleSheet("color: #0277BD;");
            
            QLabel *tempLabel = new QLabel(day["tempMin"].toString() + "~" + 
                                          day["tempMax"].toString() + "°C");
            QFont tempFont;
            tempFont.setPointSize(20);
            tempFont.setBold(true);
            tempLabel->setFont(tempFont);
            tempLabel->setStyleSheet("color: #D84315;");
            
            mainInfoLayout->addWidget(dayWeatherLabel);
            mainInfoLayout->addWidget(nightWeatherLabel);
            mainInfoLayout->addStretch();
            mainInfoLayout->addWidget(tempLabel);
            dayLayout->addLayout(mainInfoLayout);
            
            // 其他信息
            QGridLayout *detailGrid = new QGridLayout();
            detailGrid->setSpacing(10);
            
            QFont labelFont;
            labelFont.setPointSize(14);
            
            QLabel *windLabel = new QLabel("风力: " + day["windDirDay"].toString() + 
                                          " " + day["windScaleDay"].toString() + "级");
            windLabel->setFont(labelFont);
            windLabel->setStyleSheet("color: #4CAF50;");
            detailGrid->addWidget(windLabel, 0, 0);
            
            QLabel *humidityLabel = new QLabel("湿度: " + day["humidity"].toString() + "%");
            humidityLabel->setFont(labelFont);
            humidityLabel->setStyleSheet("color: #00BCD4;");
            detailGrid->addWidget(humidityLabel, 0, 1);
            
            QLabel *precipLabel = new QLabel("降水: " + day["precip"].toString() + "mm");
            precipLabel->setFont(labelFont);
            precipLabel->setStyleSheet("color: #2196F3;");
            detailGrid->addWidget(precipLabel, 1, 0);
            
            QLabel *uvLabel = new QLabel("紫外线: " + day["uvIndex"].toString());
            uvLabel->setFont(labelFont);
            uvLabel->setStyleSheet("color: #FF9800;");
            detailGrid->addWidget(uvLabel, 1, 1);
            
            dayLayout->addLayout(detailGrid);
            
            forecastDetailWidget->layout()->addWidget(dayFrame);
        }
    }
}

QString ForecastDetailPage::formatDateTime(qint64 timestamp)
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    return dt.toString("MM-dd hh:mm");
}

void ForecastDetailPage::wheelEvent(QWheelEvent *event)
{
    // qDebug() << "[ForecastDetailPage] wheelEvent triggered, delta:" << event->angleDelta().y();
    
    // 将鼠标滚轮事件转发给滚动区域的垂直滚动条
    if (scrollArea && scrollArea->verticalScrollBar()) {
        QScrollBar *scrollBar = scrollArea->verticalScrollBar();
        // 使用更自然的滚动速度
        int delta = -event->angleDelta().y() / 2;
        int oldValue = scrollBar->value();
        scrollBar->setValue(oldValue + delta);
        // qDebug() << "[ForecastDetailPage] Scroll from" << oldValue << "to" << scrollBar->value();
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void ForecastDetailPage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPos = event->pos();
        isDragging = true;
        // qDebug() << "[ForecastDetailPage] Mouse press at:" << dragStartPos;
    }
    QWidget::mousePressEvent(event);
}

void ForecastDetailPage::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        int deltaY = event->pos().y() - dragStartPos.y();
        // qDebug() << "[ForecastDetailPage] Mouse move, deltaY:" << deltaY;
        
        // 如果是垂直拖动，滚动内容
        if (qAbs(deltaY) > 5 && scrollArea && scrollArea->verticalScrollBar()) {
            QScrollBar *scrollBar = scrollArea->verticalScrollBar();
            int oldValue = scrollBar->value();
            scrollBar->setValue(oldValue - deltaY);
            dragStartPos = event->pos();
            // qDebug() << "[ForecastDetailPage] Scrolling from" << oldValue << "to" << scrollBar->value();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ForecastDetailPage::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        int deltaY = event->pos().y() - dragStartPos.y();
        // qDebug() << "[ForecastDetailPage] Mouse release, total deltaY:" << deltaY;
        isDragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void ForecastDetailPage::resetScrollPosition()
{
    if (scrollArea && scrollArea->verticalScrollBar()) {
        scrollArea->verticalScrollBar()->setValue(0);
    }
}
