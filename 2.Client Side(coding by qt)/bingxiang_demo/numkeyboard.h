#ifndef NUMKEYBOARD_H
#define NUMKEYBOARD_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>

class NumKeyboard : public QDialog
{
    Q_OBJECT

public:
    explicit NumKeyboard(const QString &title, int currentValue, int minValue, int maxValue, 
                        const QString &unit = "分钟", QWidget *parent = nullptr);
    int getValue() const { return inputValue; }

private slots:
    void onNumberClicked();
    void onBackspaceClicked();
    void onClearClicked();
    void onConfirmClicked();
    void onCancelClicked();

private:
    void setupUI(const QString &title);
    void updateDisplay();
    bool validateInput();

    QLineEdit *displayEdit;
    QLabel *rangeLabel;
    QPushButton *confirmBtn;
    
    int inputValue;
    int minValue;
    int maxValue;
    QString currentInput;
    QString unit;  // 单位（分钟、°C等）
};

#endif // NUMKEYBOARD_H
