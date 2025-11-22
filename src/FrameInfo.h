#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>

class FrameInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FrameInfoDialog(QWidget *parent = nullptr);
    ~FrameInfoDialog();

    void addTransmittedFrame(int current, int total, size_t stuffedFcsSize, const std::string& stuffedFrame);

private slots:
    void onClearText();

private:
    void setupUI();

    QTextEdit *m_textEdit;
    QPushButton *m_clearButton;
};
