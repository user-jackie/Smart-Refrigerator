#include "numkeyboard.h"
#include <QFont>
#include <QMessageBox>

NumKeyboard::NumKeyboard(const QString &title, int currentValue, int minValue, int maxValue, 
                         const QString &unit, QWidget *parent)
    : QDialog(parent), inputValue(currentValue), minValue(minValue), maxValue(maxValue), unit(unit)
{
    currentInput = QString::number(currentValue);
    setupUI(title);
    setModal(true);
    setWindowTitle(title);
}

void NumKeyboard::setupUI(const QString &title)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 设置对话框样式
    this->setStyleSheet("QDialog { background-color: #F5F5F5; }");
    this->setMinimumSize(400, 550);

    // 标题
    QLabel *titleLabel = new QLabel(title);
    QFont titleFont;
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #333; padding: 10px;");

    // 显示区域
    displayEdit = new QLineEdit();
    displayEdit->setReadOnly(true);
    displayEdit->setAlignment(Qt::AlignRight);
    displayEdit->setMinimumHeight(60);
    QFont displayFont;
    displayFont.setPointSize(32);
    displayFont.setBold(true);
    displayEdit->setFont(displayFont);
    displayEdit->setStyleSheet(
        "QLineEdit { "
        "background-color: white; "
        "border: 3px solid #9C27B0; "
        "border-radius: 8px; "
        "padding: 10px; "
        "color: #333; "
        "}"
    );
    displayEdit->setText(currentInput);

    // 范围提示
    rangeLabel = new QLabel(QString("范围：%1 - %2 %3").arg(minValue).arg(maxValue).arg(unit));
    QFont rangeFont;
    rangeFont.setPointSize(16);
    rangeLabel->setFont(rangeFont);
    rangeLabel->setAlignment(Qt::AlignCenter);
    rangeLabel->setStyleSheet("color: #666; padding: 5px;");

    // 数字键盘
    QGridLayout *keypadLayout = new QGridLayout();
    keypadLayout->setSpacing(10);

    QString buttonStyle = 
        "QPushButton { "
        "background-color: white; "
        "border: 2px solid #E0E0E0; "
        "border-radius: 8px; "
        "font-size: 28px; "
        "font-weight: bold; "
        "color: #333; "
        "min-height: 60px; "
        "}"
        "QPushButton:pressed { "
        "background-color: #E0E0E0; "
        "}";

    // 数字按钮 1-9
    for (int i = 1; i <= 9; i++) {
        QPushButton *btn = new QPushButton(QString::number(i));
        btn->setStyleSheet(buttonStyle);
        connect(btn, &QPushButton::clicked, this, &NumKeyboard::onNumberClicked);
        int row = (i - 1) / 3;
        int col = (i - 1) % 3;
        keypadLayout->addWidget(btn, row, col);
    }

    // 清除按钮
    QPushButton *clearBtn = new QPushButton("清除");
    clearBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #FF9800; "
        "border: none; "
        "border-radius: 8px; "
        "font-size: 20px; "
        "font-weight: bold; "
        "color: white; "
        "min-height: 60px; "
        "}"
        "QPushButton:pressed { "
        "background-color: #F57C00; "
        "}"
    );
    connect(clearBtn, &QPushButton::clicked, this, &NumKeyboard::onClearClicked);
    keypadLayout->addWidget(clearBtn, 3, 0);

    // 数字0
    QPushButton *btn0 = new QPushButton("0");
    btn0->setStyleSheet(buttonStyle);
    connect(btn0, &QPushButton::clicked, this, &NumKeyboard::onNumberClicked);
    keypadLayout->addWidget(btn0, 3, 1);

    // 退格按钮
    QPushButton *backspaceBtn = new QPushButton("←");
    backspaceBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #FFC107; "
        "border: none; "
        "border-radius: 8px; "
        "font-size: 28px; "
        "font-weight: bold; "
        "color: white; "
        "min-height: 60px; "
        "}"
        "QPushButton:pressed { "
        "background-color: #FFA000; "
        "}"
    );
    connect(backspaceBtn, &QPushButton::clicked, this, &NumKeyboard::onBackspaceClicked);
    keypadLayout->addWidget(backspaceBtn, 3, 2);

    // 底部按钮
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(15);

    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setMinimumHeight(55);
    cancelBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #9E9E9E; "
        "border: none; "
        "border-radius: 8px; "
        "font-size: 22px; "
        "font-weight: bold; "
        "color: white; "
        "}"
        "QPushButton:pressed { "
        "background-color: #757575; "
        "}"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &NumKeyboard::onCancelClicked);

    confirmBtn = new QPushButton("确认");
    confirmBtn->setMinimumHeight(55);
    confirmBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #4CAF50; "
        "border: none; "
        "border-radius: 8px; "
        "font-size: 22px; "
        "font-weight: bold; "
        "color: white; "
        "}"
        "QPushButton:pressed { "
        "background-color: #388E3C; "
        "}"
    );
    connect(confirmBtn, &QPushButton::clicked, this, &NumKeyboard::onConfirmClicked);

    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(confirmBtn);

    // 组装布局
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(displayEdit);
    mainLayout->addWidget(rangeLabel);
    mainLayout->addLayout(keypadLayout);
    mainLayout->addLayout(bottomLayout);
}

void NumKeyboard::onNumberClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        // 限制输入长度
        if (currentInput.length() < 3) {
            // 避免前导零
            if (currentInput == "0") {
                currentInput = btn->text();
            } else {
                currentInput += btn->text();
            }
            updateDisplay();
        }
    }
}

void NumKeyboard::onBackspaceClicked()
{
    if (!currentInput.isEmpty()) {
        currentInput.chop(1);
        if (currentInput.isEmpty()) {
            currentInput = "0";
        }
        updateDisplay();
    }
}

void NumKeyboard::onClearClicked()
{
    currentInput = "0";
    updateDisplay();
}

void NumKeyboard::onConfirmClicked()
{
    if (validateInput()) {
        inputValue = currentInput.toInt();
        accept();
    } else {
        QMessageBox::warning(this, "输入错误", 
                           QString("请输入 %1 到 %2 之间的数值").arg(minValue).arg(maxValue));
    }
}

void NumKeyboard::onCancelClicked()
{
    reject();
}

void NumKeyboard::updateDisplay()
{
    displayEdit->setText(currentInput);
    
    // 实时验证并更新确认按钮状态
    bool valid = validateInput();
    confirmBtn->setEnabled(valid);
    
    if (valid) {
        displayEdit->setStyleSheet(
            "QLineEdit { "
            "background-color: white; "
            "border: 3px solid #4CAF50; "
            "border-radius: 8px; "
            "padding: 10px; "
            "color: #333; "
            "}"
        );
    } else {
        displayEdit->setStyleSheet(
            "QLineEdit { "
            "background-color: white; "
            "border: 3px solid #F44336; "
            "border-radius: 8px; "
            "padding: 10px; "
            "color: #333; "
            "}"
        );
    }
}

bool NumKeyboard::validateInput()
{
    bool ok;
    int value = currentInput.toInt(&ok);
    return ok && value >= minValue && value <= maxValue;
}
