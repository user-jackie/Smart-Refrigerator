#ifndef STERILIZATIONPAGE_H
#define STERILIZATIONPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
        QLabel::mousePressEvent(event);
    }
};

class SterilizationPage : public QWidget
{
    Q_OBJECT

public:
    explicit SterilizationPage(QWidget *parent = nullptr);

private:
    // 紫外线杀菌
    QLabel *uvTitleLabel;
    QPushButton *uvSwitchBtn;
    ClickableLabel *uvTimeLabel;
    int uvTimeValue;
    QLabel *uvStatusLabel;
    bool uvEnabled;
    int uvRemainingTime;
    QTimer *uvTimer;

    // 空气净化
    QLabel *airTitleLabel;
    QPushButton *airSwitchBtn;
    ClickableLabel *airTimeLabel;
    int airTimeValue;
    QLabel *airStatusLabel;
    bool airEnabled;
    int airRemainingTime;
    QTimer *airTimer;

    void setupUI();
    void setGPIO(const QString &gpioPath, bool high);
    QGraphicsDropShadowEffect* createShadowEffect();

private slots:
    void onUVSwitchClicked();
    void onAirSwitchClicked();
    void onUVTimeClicked();
    void onAirTimeClicked();
    void updateUVTimer();
    void updateAirTimer();
};

#endif // STERILIZATIONPAGE_H
