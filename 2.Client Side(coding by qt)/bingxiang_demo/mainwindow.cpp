#include "mainwindow.h"
#include <QMouseEvent>
#include <QFont>
#include "connectivitypage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isDragging(false), currentPage(0)
{
    setupUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 创建堆叠窗口
    stackedWidget = new QStackedWidget();
    
    tempPage = new TemperaturePage();
    aiPage = new AIPage();
    envPage = new EnvironmentPage();
    sterilPage = new SterilizationPage();
    connectPage = new ConnectivityPage();
    
    stackedWidget->addWidget(tempPage);
    stackedWidget->addWidget(aiPage);
    stackedWidget->addWidget(envPage);
    stackedWidget->addWidget(sterilPage);
    stackedWidget->addWidget(connectPage);

    // 创建导航栏
    navigationBar = new QWidget();
    navigationBar->setFixedHeight(80);
    navigationBar->setStyleSheet("background-color: #263238;");
    
    QHBoxLayout *navLayout = new QHBoxLayout(navigationBar);
    navLayout->setSpacing(0);
    navLayout->setContentsMargins(0, 0, 0, 0);

    btn1 = new QPushButton("温度控制");
    btn2 = new QPushButton("AI助手");
    btn3 = new QPushButton("环境监测");
    btn4 = new QPushButton("杀菌净化");
    btn5 = new QPushButton("网络连接");

    QList<QPushButton*> buttons = {btn1, btn2, btn3, btn4, btn5};
    for (int i = 0; i < buttons.size(); ++i) {
        QPushButton *btn = buttons[i];
        btn->setMinimumHeight(80);
        QFont btnFont;
        btnFont.setPointSize(20);
        btn->setFont(btnFont);
        btn->setStyleSheet(
            "QPushButton { background-color: #37474F; color: white; border: none; }"
            "QPushButton:hover { background-color: #455A64; }"
            "QPushButton:pressed { background-color: #546E7A; }"
        );
        navLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, i]() { onNavButtonClicked(i); });
    }

    mainLayout->addWidget(stackedWidget);
    mainLayout->addWidget(navigationBar);

    setCentralWidget(centralWidget);
    updateNavigationButtons();
}

void MainWindow::updateNavigationButtons()
{
    QList<QPushButton*> buttons = {btn1, btn2, btn3, btn4, btn5};
    for (int i = 0; i < buttons.size(); ++i) {
        if (i == currentPage) {
            buttons[i]->setStyleSheet(
                "QPushButton { background-color: #00BCD4; color: white; border: none; font-weight: bold; }"
                "QPushButton:hover { background-color: #00ACC1; }"
            );
        } else {
            buttons[i]->setStyleSheet(
                "QPushButton { background-color: #37474F; color: white; border: none; }"
                "QPushButton:hover { background-color: #455A64; }"
                "QPushButton:pressed { background-color: #546E7A; }"
            );
        }
    }
}

void MainWindow::switchToPage(int index)
{
    if (index >= 0 && index < stackedWidget->count()) {
        currentPage = index;
        stackedWidget->setCurrentIndex(index);
        updateNavigationButtons();
    }
}

void MainWindow::onNavButtonClicked(int index)
{
    switchToPage(index);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPos = event->pos();
        isDragging = true;
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isDragging) {
        // 检查当前页面是否允许手势切换
        if (!isGestureEnabled()) {
            isDragging = false;
            QMainWindow::mouseReleaseEvent(event);
            return;
        }
        
        int deltaX = event->pos().x() - dragStartPos.x();
        
        // 滑动阈值
        if (qAbs(deltaX) > 100) {
            if (deltaX > 0) {
                // 向右滑动，切换到上一页
                if (currentPage > 0) {
                    switchToPage(currentPage - 1);
                }
            } else {
                // 向左滑动，切换到下一页
                if (currentPage < stackedWidget->count() - 1) {
                    switchToPage(currentPage + 1);
                }
            }
        }
        
        isDragging = false;
    }
    QMainWindow::mouseReleaseEvent(event);
}

bool MainWindow::isGestureEnabled() const
{
    // 如果当前在环境监测页面，检查是否在子菜单中
    if (currentPage == 2 && envPage) {  // 2 是环境监测页面的索引
        return envPage->isOnMainPage();
    }
    // 其他页面默认允许手势切换
    return true;
}

