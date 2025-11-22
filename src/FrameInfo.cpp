#include "FrameInfo.h"
#include "Frame.h"
#include "FrameManager.h"
#include <QDateTime>
#include <QScrollBar>

FrameInfoDialog::FrameInfoDialog(QWidget *parent)
    : QDialog()
{
    setupUI();
    setWindowTitle("Структура переданных кадров");
    setFixedSize(780, 400);

    setModal(false);
}

FrameInfoDialog::~FrameInfoDialog()
{
}

void FrameInfoDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setFont(QFont("Courier New", 9));
    m_textEdit->setReadOnly(true);

    m_textEdit->setPlaceholderText("Здесь будет выведена структура переданных кадров...");

    m_clearButton = new QPushButton("Очистить", this);
    connect(m_clearButton, &QPushButton::clicked, this, &FrameInfoDialog::onClearText);
    m_clearButton->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();

    mainLayout->addWidget(new QLabel("Структура переданных кадров:"));
    mainLayout->addWidget(m_textEdit);
    mainLayout->addLayout(buttonLayout);
}

void FrameInfoDialog::addTransmittedFrame(int current, int total, size_t stuffedFcsSize, const std::string& stuffedFrame)
{
    QString frameStructure;
    frameStructure += QString("[%1] Кадр %2/%3\n")
                          .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                          .arg(current)
                          .arg(total);

    std::vector<uint8_t> stuffedBytes(stuffedFrame.begin(), stuffedFrame.end());

    size_t counter = 0;

    frameStructure += QString("  Флаг начала:        0x%1\n").arg(static_cast<uint8_t>(stuffedBytes[counter++]), 2, 16, QLatin1Char('0'));
    if (stuffedBytes[counter] != ESCAPE_BYTE) {
        frameStructure += QString("  Всего кадров:       0x%1\n").arg(static_cast<uint8_t>(stuffedBytes[counter++]), 2, 16, QLatin1Char('0'));
    }
    else {
        frameStructure += QString("  Всего кадров:       0x%1 0x%2\n").arg(static_cast<uint8_t>(stuffedBytes[counter++]), 2, 16, QLatin1Char('0'))
                                                              .arg(static_cast<uint8_t>(stuffedBytes[counter++]), 2, 16, QLatin1Char('0'));
    }
    if(stuffedBytes[counter] != ESCAPE_BYTE) {
        frameStructure += QString("  Номер кадра:        0x%1\n").arg(static_cast<uint8_t>(stuffedBytes[counter++]), 2, 16, QLatin1Char('0'));
    }
    else {
        frameStructure += QString("  Номер кадра:        0x%1 0x%2\n").arg(static_cast<uint8_t>(stuffedBytes[counter++]), 2, 16, QLatin1Char('0'))
                                                              .arg(static_cast<uint8_t>(stuffedBytes[counter++]), 2, 16, QLatin1Char('0'));
    }

    frameStructure += "  Данные:             ";
    for (size_t i = 0; i + counter < stuffedFrame.size() - 1 - stuffedFcsSize; i++) {
        frameStructure += QString("0x%1 ").arg(static_cast<uint8_t>(stuffedFrame[i + counter]), 2, 16, QLatin1Char('0'));
        if ((i + 1) % 16 == 0 && i + 1 + counter < stuffedFrame.size() - 1 - stuffedFcsSize) {
            frameStructure += "\n                      ";
        }
    }
    frameStructure += "\n";

    frameStructure += "  Контрольная сумма:  ";
    size_t fcsStartPos = stuffedFrame.size() - stuffedFcsSize - 1;

    for (size_t i = 0; i < stuffedFcsSize; i++) {
        frameStructure += QString("0x%1 ").arg(static_cast<uint8_t>(stuffedFrame[fcsStartPos + i]), 2, 16, QLatin1Char('0'));
    }
    frameStructure += "\n";

    frameStructure += QString("  Флаг конца:         0x%1\n").arg(static_cast<uint8_t>(stuffedFrame.back()), 2, 16, QLatin1Char('0'));

    frameStructure += "─────────────────────────────────────────────────────────────────────────────────────────────────────\n";

    QScrollBar *scrollBar = m_textEdit->verticalScrollBar();
    bool atBottom = scrollBar->value() == scrollBar->maximum();

    m_textEdit->append(frameStructure);

    if (atBottom) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void FrameInfoDialog::onClearText()
{
    m_textEdit->clear();
}
