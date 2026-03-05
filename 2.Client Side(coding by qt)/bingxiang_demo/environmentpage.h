#ifndef ENVIRONMENTPAGE_H
#define ENVIRONMENTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QTcpSocket>
#include <QPushButton>
#include <QSpinBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStackedWidget>
#include <QFrame>
#include <QComboBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>

// 前向声明
class WeatherDetailPage;
class ForecastDetailPage;

class EnvironmentPage : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentPage(QWidget *parent = nullptr);
    ~EnvironmentPage();
    
    // 判断是否在主页面（用于禁用主窗口的手势切换）
    bool isOnMainPage() const { return stackedWidget->currentWidget() == mainPage; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // 页面切换
    QStackedWidget *stackedWidget;
    QWidget *mainPage;
    WeatherDetailPage *weatherDetailPage;
    ForecastDetailPage *forecastDetailPage;
    
    // 主页面 - 控制区域
    QPushButton *refreshButton;
    QComboBox *cityComboBox;
    QSpinBox *intervalSpinBox;
    QPushButton *setIntervalButton;
    
    // 主页面 - 左侧框架
    QFrame *leftFrame;
    QLabel *leftTitleLabel;
    QLabel *tempSummaryLabel;
    QLabel *weatherSummaryLabel;
    QLabel *aqiSummaryLabel;
    
    // 主页面 - 右侧框架
    QFrame *rightFrame;
    QLabel *rightTitleLabel;
    QLabel *precipSummaryLabel;
    QWidget *forecastSummaryWidget;
    
    // TCP客户端
    QTcpSocket *tcpSocket;
    QTimer *reconnectTimer;
    
    QString serverHost;
    int serverPort;
    bool isConnected;
    
    // 数据缓存
    QJsonObject weatherData;
    
    void setupUI();
    void setupMainPage();
    void setupTcpClient();
    void connectToServer();
    void parseWeatherData(const QJsonObject &data);
    void updateMainPageSummary();
    QString formatDateTime(qint64 timestamp);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void requestWeatherData();
    void onLeftFrameClicked();
    void onRightFrameClicked();
    void onBackToMain();
    void onRefreshClicked();
    void onCityChanged(int index);
    void onSetIntervalClicked();
};

// 左侧详细页面：实时天气 + 空气质量
class WeatherDetailPage : public QWidget
{
    Q_OBJECT

public:
    explicit WeatherDetailPage(QWidget *parent = nullptr);
    void updateData(const QJsonObject &data);
    void resetScrollPosition();  // 重置滚动位置到顶部

signals:
    void backClicked();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPushButton *backButton;
    QLabel *cityLabel;
    QLabel *updateTimeLabel;
    QScrollArea *scrollArea;
    
    // 鼠标拖动相关
    QPoint dragStartPos;
    bool isDragging;
    
    // 实时天气详情
    QLabel *tempValueLabel;
    QLabel *feelsLikeValueLabel;
    QLabel *weatherTextLabel;
    QLabel *humidityValueLabel;
    QLabel *windValueLabel;
    QLabel *pressureValueLabel;
    QLabel *visibilityValueLabel;
    
    // 空气质量详情
    QLabel *aqiValueLabel;
    QLabel *aqiCategoryLabel;
    QLabel *pm25ValueLabel;
    QLabel *pm10ValueLabel;
    QLabel *no2ValueLabel;
    QLabel *so2ValueLabel;
    QLabel *coValueLabel;
    QLabel *o3ValueLabel;
    
    void setupUI();
    QString formatDateTime(qint64 timestamp);
};

// 右侧详细页面：降水预报 + 3天预报
class ForecastDetailPage : public QWidget
{
    Q_OBJECT

public:
    explicit ForecastDetailPage(QWidget *parent = nullptr);
    void updateData(const QJsonObject &data);
    void resetScrollPosition();  // 重置滚动位置到顶部

signals:
    void backClicked();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPushButton *backButton;
    QLabel *cityLabel;
    QLabel *updateTimeLabel;
    QScrollArea *scrollArea;
    
    // 鼠标拖动相关
    QPoint dragStartPos;
    bool isDragging;
    
    // 降水预报详情
    QLabel *precipSummaryLabel;
    QWidget *precipDetailWidget;
    
    // 3天预报详情
    QWidget *forecastDetailWidget;
    
    void setupUI();
    QString formatDateTime(qint64 timestamp);
};

#endif // ENVIRONMENTPAGE_H
