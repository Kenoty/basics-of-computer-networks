#pragma once

#include <QMainWindow>
#include "ComPort.h"

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

    void onPortChanged(int index);
    void onBaudRateChanged(int index);

    void onDataReceived(const std::string& data);

private:
    void updatePortStatus();
    void logMessage(const QString &message, bool isIncoming = false);
    void displayReceivedData(const QString &data);

    Ui::MainWindow *ui;
    ComPort m_comPort;
    bool m_portOpened = false;
};
