#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include "temperaturepage.h"
#include "aipage.h"
#include "environmentpage.h"
#include "sterilizationpage.h"
#include "connectivitypage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // 判断当前页面是否允许手势切换
    bool isGestureEnabled() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QStackedWidget *stackedWidget;
    QWidget *navigationBar;
    
    TemperaturePage *tempPage;
    AIPage *aiPage;
    EnvironmentPage *envPage;
    SterilizationPage *sterilPage;
    ConnectivityPage *connectPage;
    
    QPushButton *btn1;
    QPushButton *btn2;
    QPushButton *btn3;
    QPushButton *btn4;
    QPushButton *btn5;
    
    QPoint dragStartPos;
    bool isDragging;
    int currentPage;
    
    void setupUI();
    void updateNavigationButtons();
    void switchToPage(int index);

private slots:
    void onNavButtonClicked(int index);
};

#endif // MAINWINDOW_H
