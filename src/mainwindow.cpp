#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QVector<int> standardBaudRates = {50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    for (int rate : standardBaudRates) {
        ui->baudRateComboBox->addItem(QString::number(rate), rate);
    }
    ui->baudRateComboBox->setCurrentIndex(11);

    onRefreshPorts();

    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
    connect(ui->openCloseButton, &QPushButton::clicked, this, &MainWindow::onOpenClose);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendData);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::onClearLog);
    connect(ui->clearReceiveButton, &QPushButton::clicked, this, &MainWindow::onClearReceive);
    connect(ui->portComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPortChanged);
    connect(ui->baudRateComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onBaudRateChanged);

    connect(ui->sendLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendData);

    int clearButtonWidth = 140;
    ui->clearButton->setFixedWidth(clearButtonWidth);
    ui->clearReceiveButton->setFixedWidth(clearButtonWidth);

    updatePortStatus();
}

MainWindow::~MainWindow()
{
    if (m_portOpened) {
        m_comPort.close();
    }
    delete ui;
}

void MainWindow::onRefreshPorts()
{
    ui->portComboBox->clear();

    auto ports = ComPort::getAvailablePorts();
    for (const auto& port : ports) {
        ui->portComboBox->addItem(QString::fromStdString(port));
    }

    if (ports.empty()) {
        ui->portComboBox->addItem("Нет доступных портов");
        ui->openCloseButton->setEnabled(false);
    } else {
        ui->openCloseButton->setEnabled(true);
    }
}

void MainWindow::onOpenClose()
{
    if (m_portOpened) {
        m_comPort.close();
        m_portOpened = false;
        logMessage("Порт закрыт");
    } else {
        QString portName = ui->portComboBox->currentText();
        int baudRate = ui->baudRateComboBox->currentData().toInt();

        if (m_comPort.open(portName.toStdString(), baudRate)) {
            m_comPort.startAsyncReading([this](const std::string& data) {
                QMetaObject::invokeMethod(this, "onDataReceived",
                                          Qt::QueuedConnection, Q_ARG(std::string, data));
            });

            m_portOpened = true;
            logMessage("Порт " + portName + " открыт, скорость: " + QString::number(baudRate));
        } else {
            QMessageBox::warning(this, "Ошибка",
                                 "Не удалось открыть порт " + portName);
        }
    }

    updatePortStatus();
}

void MainWindow::onSendData()
{
    if (!m_portOpened) {
        QMessageBox::warning(this, "Ошибка", "Порт не открыт");
        return;
    }

    QString message = ui->sendLineEdit->text();
    if (message.isEmpty()) {
        QMessageBox::information(this, "Информация", "Введите сообщение для отправки");
        return;
    }

    if (!message.endsWith("\n")) {
        message += "\n";
    }

    std::vector<Frame> frames = m_frameManager.packMessage(message.toStdString());
    logMessage("Сообщение: " + message.mid(0, message.length() - 1) + " было фрагментировано", false);

    std::vector<std::string> stuffedFrames = m_frameManager.byteStuff(frames);

    for(int i = 0; i < stuffedFrames.size(); i++) {
        if (m_comPort.writeData(stuffedFrames[i])) {
            logMessage("Отправлен кадр " + QString::number(i + 1) +
                            " из " + QString::number(frames[i].getTotal()), false);
            ui->sendLineEdit->clear();
        } else {
            logMessage("Ошибка отправки кадра", false);
        }
    }
}

void MainWindow::onDataReceived(const std::string& data)
{
    if (!data.empty()) {

        m_receivedBytes.append(data);

        if(FrameManager::isValidFrame(m_receivedBytes)) {
            Frame receivedFrame = m_frameManager.byteUnstuff(m_receivedBytes);

            std::string receivedMessage = m_frameManager.unpackMessage(receivedFrame);

            QString received = QString::fromStdString(receivedMessage);
            displayReceivedData(received);

            if(received != "\n") {
                logMessage("Получен кадр " + QString::number(receivedFrame.getSequence()) +
                               " из " + QString::number(receivedFrame.getTotal()) +
                               " с сообщением: " + received, true);
            }

            m_receivedBytes.clear();
        }
    }
}

void MainWindow::displayReceivedData(const QString &data)
{
    QScrollBar *scrollBar = ui->receiveTextEdit->verticalScrollBar();
    bool atBottom = scrollBar->value() == scrollBar->maximum();

    ui->receiveTextEdit->insertPlainText(data);

    if (atBottom) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void MainWindow::onClearLog()
{
    ui->logTextEdit->clear();
}

void MainWindow::onClearReceive()
{
    ui->receiveTextEdit->clear();
}

void MainWindow::onPortChanged(int index)
{
    Q_UNUSED(index)
    // Можно добавить дополнительную логику при смене порта
}

void MainWindow::onBaudRateChanged(int index)
{
    if (m_portOpened && index >= 0) {
        int newBaudRate = ui->baudRateComboBox->currentData().toInt();
        if (m_comPort.setBaudRate(newBaudRate)) {
            logMessage("Скорость изменена на: " + QString::number(newBaudRate));
        }
    }
}

void MainWindow::updatePortStatus()
{
    if (m_portOpened) {
        ui->openCloseButton->setText("Закрыть порт");
        ui->statusLabel->setText("✓ Порт открыт: " +
                                 QString::fromStdString(m_comPort.getPortName()) + " (" +
                                 QString::number(m_comPort.getBaudRate()) + " бод)");
        ui->statusLabel->setStyleSheet("color: green;");

        ui->sendButton->setEnabled(true);
        ui->sendLineEdit->setEnabled(true);
        ui->portComboBox->setEnabled(false);
        ui->baudRateComboBox->setEnabled(false);
        ui->refreshButton->setEnabled(false);
    } else {
        ui->openCloseButton->setText("Открыть порт");
        ui->statusLabel->setText("✗ Порт закрыт");
        ui->statusLabel->setStyleSheet("color: red;");

        ui->sendButton->setEnabled(false);
        ui->sendLineEdit->setEnabled(false);
        ui->portComboBox->setEnabled(true);
        ui->baudRateComboBox->setEnabled(true);
        ui->refreshButton->setEnabled(true);
    }
}

void MainWindow::logMessage(const QString &message, bool isIncoming)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage;

    if (isIncoming) {
        formattedMessage = QString("[%1] ← %2").arg(timestamp, message);
    } else {
        formattedMessage = QString("[%1] → %2").arg(timestamp, message);
    }

    QScrollBar *scrollBar = ui->logTextEdit->verticalScrollBar();
    bool atBottom = scrollBar->value() == scrollBar->maximum();

    ui->logTextEdit->append(formattedMessage);

    if (atBottom) {
        scrollBar->setValue(scrollBar->maximum());
    }
}
