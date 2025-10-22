#pragma once

#include <QMainWindow>
#include "ComPort.h"
#include "FrameManager.h"
#include "FrameInfo.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRefreshPorts();
    void onOpenClose();
    void onSendData();
    void onClearLog();
    void onClearReceive();
    void onShowFrameInfo();

    void onPortChanged(int index);
    void onBaudRateChanged(int index);

    void onDataReceived(const std::string& data);

    void onFrameSent(int current, int total, const std::string& stuffedFrame);
    void onSendCompleted();

    void logMessage(const QString &message, bool isIncoming);
private:
    void updatePortStatus();
    void displayReceivedData(const QString &data);
    void sendMessageInBackground(const QString& message);
    size_t findCompleteFrame(const std::string& data);

    Ui::MainWindow *ui;
    ComPort m_comPort;
    FrameManager m_frameManager;
    std::string m_receivedBytes;
    FrameInfoDialog *m_frameInfoDialog;
    bool m_portOpened = false;
    QThread* m_sendThread = nullptr;
};
