#ifndef TEMPERATUREPAGE_H
#define TEMPERATUREPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QRegExp>
#include <QtMath>
#include <QEvent>
#include <QMouseEvent>

extern "C" {
#include "ds18b20_App.h"
}

// 温度读取线程
class TempReadThread : public QThread
{
    Q_OBJECT

public:
    explicit TempReadThread(QObject *parent = nullptr);
    ~TempReadThread();
    void stopThread();

protected:
    void run() override;

signals:
    void temperatureUpdated(QString temp0, QString temp1);

private:
    bool running;
    QMutex mutex;
};

class TemperaturePage : public QWidget
{
    Q_OBJECT

public:
    explicit TemperaturePage(QWidget *parent = nullptr);
    ~TemperaturePage();

private:
    // 制热控件
    QLabel *heatTitleLabel;
    QLabel *heatTempLabel;
    QSlider *heatSlider;
    QPushButton *heatStartBtn;   // 开启制热按钮
    QPushButton *heatStopBtn;    // 停止制热按钮
    bool heatEnabled;

    // 制冷控件
    QLabel *coolTitleLabel;
    QLabel *coolTempLabel;
    QSlider *coolSlider;
    QPushButton *coolStartBtn;   // 开启制冷按钮
    QPushButton *coolStopBtn;    // 停止制冷按钮
    bool coolEnabled;

    // 实际温度显示
    QLabel *actualTempLabel0;
    QLabel *actualTempLabel1;
    
    // 温度读取线程
    TempReadThread *tempThread;
    
    // 上一次有效温度（用于滤波）
    float lastValidTemp0;
    float lastValidTemp1;
    bool hasValidTemp0;
    bool hasValidTemp1;
    
    // 温度控制
    void controlHeating(float currentTemp);
    void controlCooling(float currentTemp);
    void setHeatingPin(bool on);
    void setCoolingPin(bool on);
    void updateHeatButtons();
    void updateCoolButtons();
    
    bool heatingPinState;  // 加热引脚状态
    bool coolingPinState;  // 制冷引脚状态

    void setupUI();
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showHeatTempInput();
    void showCoolTempInput();

private slots:
    void onHeatSliderChanged(int value);
    void onCoolSliderChanged(int value);
    void onHeatStartClicked();
    void onHeatStopClicked();
    void onCoolStartClicked();
    void onCoolStopClicked();
    void onTemperatureUpdated(QString temp0, QString temp1);
};

#endif // TEMPERATUREPAGE_H
