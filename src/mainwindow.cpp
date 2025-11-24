#include "mainwindow.h"
#include "ChannelManager.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_frameInfoDialog(new FrameInfoDialog(this))
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
    connect(ui->frameInfoButton, &QPushButton::clicked, this, &MainWindow::onShowFrameInfo);
    connect(ui->enableEmulationCheckBox, &QCheckBox::toggled, this, &MainWindow::onEmulationToggled);

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
        logMessage("Порт закрыт", false);
    } else {
        QString portName = ui->portComboBox->currentText();
        int baudRate = ui->baudRateComboBox->currentData().toInt();

        if (m_comPort.open(portName.toStdString(), baudRate)) {
            m_comPort.startAsyncReading([this](const std::string& data) {
                QMetaObject::invokeMethod(this, "onDataReceived",
                                          Qt::QueuedConnection, Q_ARG(std::string, data));
            });

            m_portOpened = true;
            logMessage("Порт " + portName + " открыт, скорость: " + QString::number(baudRate),false);
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

    ui->sendButton->setEnabled(false);
    ui->sendLineEdit->setEnabled(false);

    sendMessageInBackground(message);
}

void MainWindow::sendMessageInBackground(const QString& message) {
    bool emulationEnabled = m_emulationEnabled;

    m_sendThread = QThread::create([this, message, emulationEnabled]() {
        std::vector<Frame> frames = m_frameManager.packMessage(message.toStdString());

        QMetaObject::invokeMethod(this, "logMessage",
                                  Qt::QueuedConnection,
                                  Q_ARG(const QString&, "Сообщение сегментировано на " + QString::number(frames.size()) + " кадров"),
                                  Q_ARG(bool, false));

        std::vector<std::string> stuffedFrames = m_frameManager.byteStuff(frames);

        for(int i = 0; i < stuffedFrames.size(); i++) {
            bool success;

            if (emulationEnabled) {
                success = transmitWithCSMACD(stuffedFrames[i], i + 1);
            } else {
                success = m_comPort.writeData(stuffedFrames[i]);
            }

            if (success) {
                QMetaObject::invokeMethod(this, "onFrameSent",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, i + 1),
                                          Q_ARG(int, frames[i].getTotal()),
                                          Q_ARG(size_t, m_frameManager.getStuffedFcsSize(frames[i].getFcs())),
                                          Q_ARG(std::string, stuffedFrames[i]));

                QMetaObject::invokeMethod(this, "logMessage",
                                          Qt::QueuedConnection,
                                          Q_ARG(const QString&, "Кадр " + QString::number(i + 1) + " успешно передан"),
                                          Q_ARG(bool, false));
            } else {
                QString errorMsg = emulationEnabled ?
                                       "Ошибка: не удалось передать кадр " + QString::number(i + 1) + " после всех попыток" :
                                       "Ошибка передачи кадра " + QString::number(i + 1);

                QMetaObject::invokeMethod(this, "logMessage",
                                          Qt::QueuedConnection,
                                          Q_ARG(const QString&, errorMsg),
                                          Q_ARG(bool, false));
            }

            QThread::msleep(10);
        }

        QMetaObject::invokeMethod(this, "onSendCompleted", Qt::QueuedConnection);
    });

    connect(m_sendThread, &QThread::finished, m_sendThread, &QObject::deleteLater);
    m_sendThread->start();
}

size_t MainWindow::findCompleteFrame(const std::string& data)
{
    if (data.length() < HEADER_SIZE + 1 + 1 + TRAILER_SIZE) {
        return 0;
    }

    size_t startPos = data.find(static_cast<char>(START_FLAG_BYTE));
    if (startPos == std::string::npos) {
        return 0;
    }

    while (startPos < data.length() && data[startPos] == static_cast<char>(START_FLAG_BYTE)) {
        startPos++;
    }
    if (startPos > 0) startPos--;

    for (size_t i = startPos + 1; i < data.length(); i++) {
        if (static_cast<uint8_t>(data[i]) == END_FLAG_BYTE) {
            size_t frameLength = i - startPos + 1;

            std::string potentialFrame = data.substr(startPos, frameLength);
            if (FrameManager::isValidFrame(potentialFrame)) {
                return startPos + frameLength;
            }
        }
    }

    return 0;
}

void MainWindow::onDataReceived(const std::string& data)
{
    if (!data.empty()) {
        if (static_cast<uint8_t>(data[0]) == START_FLAG_BYTE && !m_receivedBytes.empty()) {
            m_receivedBytes.clear();
        }

        std::string jamPattern(4, '\xFF');
        size_t jamPos = data.find(jamPattern);

        if (jamPos != std::string::npos) {
            logMessage("Обнаружен JAM-сигнал", true);

            if (jamPos > 0) {
                std::string beforeJam = data.substr(0, jamPos);
                m_receivedBytes.append(beforeJam);
            }

            size_t afterJamPos = jamPos + jamPattern.length();
            if (afterJamPos < data.length()) {
                std::string afterJam = data.substr(afterJamPos);
                m_receivedBytes.append(afterJam);
            }

        } else {
            m_receivedBytes.append(data);
        }

        while (true) {
            size_t frameLength = findCompleteFrame(m_receivedBytes);
            if (frameLength == 0) {
                break;
            }

            std::string receivedFrame = m_receivedBytes.substr(0, frameLength);
            m_receivedBytes = m_receivedBytes.substr(frameLength);

            Frame unstaffedFrame = m_frameManager.byteUnstuff(receivedFrame);

            unstaffedFrame.simulateErrors();

            QString corruptedReceivedMessage = QString::fromStdString(m_frameManager.unpackMessage(unstaffedFrame));

            if(corruptedReceivedMessage != "\n") {
                logMessage("Получен кадр " + QString::number(unstaffedFrame.getSequence()) +
                               " из " + QString::number(unstaffedFrame.getTotal()) +
                               " с данными: " + corruptedReceivedMessage, true);
            }

            //logMessage("Сгенерировано ошибок в " + QString::number(unstaffedFrame.simulateErrors()) + " битах", true);

            switch(unstaffedFrame.correctErrors()) {
            case 0:
                logMessage("Ошибок обнаружено не было", true);
                break;
            case 1:
                logMessage("Исправлена одиночная ошибка", true);
                break;
            case 2:
                logMessage("Обнаружена двойная ошибка", true);
                break;
            default:
                logMessage("Были переданы пустые данные", true);
            }

            QString receivedMessage = QString::fromStdString(m_frameManager.unpackMessage(unstaffedFrame));
            displayReceivedData(receivedMessage);

            if(unstaffedFrame.getSequence() == unstaffedFrame.getTotal()) {
                displayReceivedData("\n");
            }
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
            logMessage("Скорость изменена на: " + QString::number(newBaudRate), false);
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

void MainWindow::onShowFrameInfo()
{
    m_frameInfoDialog->show();
    m_frameInfoDialog->raise();
    m_frameInfoDialog->activateWindow();
}

void MainWindow::onFrameSent(int current, int total, size_t stuffedFcsSize, const std::string& stuffedFrame)
{
    logMessage("Отправлен кадр " + QString::number(current) + " из " + QString::number(total), false);
    m_frameInfoDialog->addTransmittedFrame(current, total, stuffedFcsSize, stuffedFrame);
}

void MainWindow::onSendCompleted()
{
    ui->sendLineEdit->clear();
    ui->sendButton->setEnabled(true);
    ui->sendLineEdit->setEnabled(true);

    if (m_sendThread) {
        m_sendThread->quit();
        m_sendThread->wait();
        m_sendThread = nullptr;
    }
}

// Основной метод передачи с CSMA/CD
bool MainWindow::transmitWithCSMACD(const std::string& frameData, int frameNumber) {
    ChannelManager& channel = ChannelManager::getInstance();
    int attempt = 0;
    const int maxAttempts = channel.getMaxAttempts();
    const double slotTime = channel.getSlotTime();

    while (attempt < maxAttempts) {
        if (channel.isChannelBusy()) {
            int backoffSlots = channel.calculateBackoffDelay(attempt);
            int backoffMs = static_cast<int>(backoffSlots * slotTime);

            QMetaObject::invokeMethod(this, "logMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(const QString&,
                                            QString("Канал занят, попытка %1, Задержка: %2 слотов (%3 мс)")
                                                .arg(attempt + 1).arg(backoffSlots).arg(backoffMs)),
                                      Q_ARG(bool, false));

            QThread::msleep(backoffMs);
            attempt++;
            continue;
        }

        bool collisionDetected = false;

        for (size_t i = 0; i < frameData.length(); i++) {
            if (channel.isCollisionOccurred()) {
                collisionDetected = true;
                break;
            }

            std::string byteData(1, frameData[i]);
            if (!m_comPort.writeData(byteData)) {
                collisionDetected = true;
                break;
            }

            QThread::msleep(static_cast<int>(slotTime / frameData.length()));
        }

        if (collisionDetected) {
            // Коллизия обнаружена на одном из байтов - отправляем jam-сигнал
            sendJamSignal();

            int backoffSlots = channel.calculateBackoffDelay(attempt);
            int backoffMs = static_cast<int>(backoffSlots * slotTime);

            QMetaObject::invokeMethod(this, "logMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(const QString&,
                                            QString("Коллизия на байте! Задержка: %1 слотов (%2 мс)")
                                                .arg(backoffSlots).arg(backoffMs)),
                                      Q_ARG(bool, false));

            QThread::msleep(backoffMs);
            attempt++;
        } else {
            // Успешная передача всего кадра без коллизий
            return true;
        }
    }

    return false;
}

void MainWindow::sendJamSignal() {
    // 32 бита jam-сигнала (4 байта)
    std::vector<uint8_t> jamSignal(JAM_SIGNAL_SIZE / 8, 0xFF);
    std::string jamStr(jamSignal.begin(), jamSignal.end());

    m_comPort.writeData(jamStr);

    QMetaObject::invokeMethod(this, "logMessage",
                              Qt::QueuedConnection,
                              Q_ARG(const QString&, "Отправлен JAM-сигнал"),
                              Q_ARG(bool, false));
}

void MainWindow::onEmulationToggled(bool checked) {
    m_emulationEnabled = checked;
    ChannelManager::getInstance().setEmulationEnabled(checked);

    if (checked) {
        logMessage("Эмуляция CSMA/CD включена", false);
    } else {
        logMessage("Эмуляция CSMA/CD выключена", false);
    }
}
