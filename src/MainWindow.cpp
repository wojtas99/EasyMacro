#include "MainWindow.h"

#include "MacroRecorder.h"
#include "MacroPlayer.h"
#include "SettingsDialog.h"
#include "LicenseDialog.h"
#include "MacroEvent.h"

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QToolButton>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("EasyMacro"));

    m_recorder = new MacroRecorder(this);
    m_player = new MacroPlayer(this);

    buildUi();
    buildMenu();
    applyHotkeys();

    connect(m_recorder, &MacroRecorder::eventRecorded, this, &MainWindow::onEventRecorded);
    connect(m_recorder, &MacroRecorder::runHotkeyPressed, this, &MainWindow::onRunHotkey);
    connect(m_recorder, &MacroRecorder::stopHotkeyPressed, this, &MainWindow::onStopHotkey);
    connect(m_recorder, &MacroRecorder::recordHotkeyPressed, this, &MainWindow::onRecordHotkey);

    connect(m_player, &MacroPlayer::playbackStarted, this, &MainWindow::onPlaybackStarted);
    connect(m_player, &MacroPlayer::playbackFinished, this, &MainWindow::onPlaybackFinished);
    connect(m_player, &MacroPlayer::iterationChanged, this, &MainWindow::onIterationChanged);

    if (!m_recorder->installHooks()) {
        QMessageBox::critical(this, tr("EasyMacro"),
                              tr("Failed to install the input hooks. Recording and hotkeys are disabled."));
    }

    updateControls();
}

MainWindow::~MainWindow() {
    if (m_player->isRunning()) {
        m_player->requestStop();
        m_player->wait();
    }
    m_recorder->removeHooks();
}

void MainWindow::buildUi() {
    auto *central = new QWidget(this);
    central->setContentsMargins(12, 10, 12, 12);

    m_recordButton = new QPushButton(tr("Record"), central);
    m_playButton = new QPushButton(tr("Play"), central);
    m_clearButton = new QPushButton(tr("Clear List"), central);
    m_recordButton->setFixedHeight(26);
    m_playButton->setFixedHeight(26);
    m_clearButton->setFixedHeight(26);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);
    buttonRow->addWidget(m_recordButton);
    buttonRow->addWidget(m_playButton);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_clearButton);

    m_actionList = new QTableWidget(0, 4, central);
    m_actionList->setHorizontalHeaderLabels({tr("#"), tr("Type"), tr("Delay (ms)"), tr("Del")});
    m_actionList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_actionList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_actionList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_actionList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_actionList->setColumnWidth(0, 40);
    m_actionList->setColumnWidth(2, 68);
    m_actionList->setColumnWidth(3, 40);
    m_actionList->verticalHeader()->setVisible(false);
    m_actionList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    m_actionList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_actionList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_actionList->setAlternatingRowColors(true);
    m_actionList->setShowGrid(false);
    m_actionList->verticalHeader()->setDefaultSectionSize(28);

    auto *layout = new QVBoxLayout(central);
    layout->setSpacing(10);
    layout->addWidget(m_actionList);
    layout->addLayout(buttonRow);

    setCentralWidget(central);

    connect(m_recordButton, &QPushButton::clicked, this, &MainWindow::toggleRecording);
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(m_clearButton, &QPushButton::clicked, this, [this]() {
        m_recorder->setEvents({});
        onEventRecorded(0);
        updateActionList();
        updateControls();
    });
    connect(m_actionList, &QTableWidget::cellChanged, this, &MainWindow::onCellChanged);

    setFixedSize(300, 200);
}

void MainWindow::buildMenu() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Save..."), this, &MainWindow::saveMacro);
    fileMenu->addAction(tr("&Load..."), this, &MainWindow::loadMacro);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Se&ttings..."), this, &MainWindow::openSettings);
}

void MainWindow::applyHotkeys() {
    m_recorder->setHotkeys(m_settings.runHotkeyVk(), m_settings.stopHotkeyVk(), m_settings.recordHotkeyVk());
}

void MainWindow::toggleRecording() {
    if (m_recorder->isRecording()) {
        stopRecording();
    } else {
        startRecording();
    }
}

void MainWindow::togglePlayback() {
    if (m_playing) {
        stopPlayback();
    } else {
        startPlayback();
    }
}

void MainWindow::startRecording() {
    if (m_playing || m_recorder->isRecording()) return;
    m_recorder->startRecording();
    onEventRecorded(0);
    updateActionList();
    updateControls();
}

void MainWindow::stopRecording() {
    if (!m_recorder->isRecording()) return;
    m_recorder->stopRecording();
    updateActionList();
    updateControls();
}

void MainWindow::startPlayback() {
    if (m_playing || m_recorder->isRecording() || m_player->isRunning()) return;
    if (m_recorder->eventCount() == 0) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Nothing to play. Record or load a macro first."));
        return;
    }
    m_player->configure(m_recorder->events(), m_settings.repeatCount(), m_settings.repeatInfinite(), m_settings.instantSpeed());
    m_player->start();
}

void MainWindow::stopPlayback() {
    if (!m_playing) return;
    m_player->requestStop();
}

void MainWindow::saveMacro() {
    if (m_recorder->isRecording()) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Stop recording before saving."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Macro"), QString(),
                                                      tr("EasyMacro Files (*.emacro);;JSON Files (*.json)"));
    if (path.isEmpty()) return;

    QJsonArray array;
    for (const MacroEvent &e : m_recorder->events()) array.append(e.toJson());
    QJsonObject root;
    root.insert("version", 1);
    root.insert("events", array);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("EasyMacro"), tr("Could not open file for writing."));
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::loadMacro() {
    if (m_recorder->isRecording() || m_playing) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Stop the current action before loading."));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Load Macro"), QString(),
                                                      tr("EasyMacro Files (*.emacro);;JSON Files (*.json)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("EasyMacro"), tr("Could not open file for reading."));
        return;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, tr("EasyMacro"), tr("Invalid macro file."));
        return;
    }

    const QJsonArray array = doc.object().value("events").toArray();
    QVector<MacroEvent> events;
    events.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (value.isObject()) events.append(MacroEvent::fromJson(value.toObject()));
    }
    m_recorder->setEvents(events);
    onEventRecorded(events.size());
    updateActionList();
    updateControls();
}

void MainWindow::openSettings() {
    if (m_recorder->isRecording() || m_playing) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Stop the current action before changing settings."));
        return;
    }
    SettingsDialog dialog(m_settings, this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_settings.setRunHotkeyVk(dialog.runHotkeyVk());
    m_settings.setStopHotkeyVk(dialog.stopHotkeyVk());
    m_settings.setRecordHotkeyVk(dialog.recordHotkeyVk());
    m_settings.setRepeatCount(dialog.repeatCount());
    m_settings.setRepeatInfinite(dialog.repeatInfinite());
    m_settings.setInstantSpeed(dialog.instantSpeed());
    m_settings.save();
    applyHotkeys();
}

void MainWindow::showLicense() {
    LicenseDialog dialog(this);
    dialog.exec();
}

void MainWindow::removeAction(int row) {
    QVector<MacroEvent> events = m_recorder->events();
    if (row < 0 || row >= events.size()) return;
    events.remove(row);
    m_recorder->setEvents(events);
    onEventRecorded(events.size());
    updateActionList();
}

void MainWindow::updateActionList() {
    m_actionList->blockSignals(true);
    const QVector<MacroEvent> &events = m_recorder->events();
    m_actionList->setRowCount(0);
    for (int i = 0; i < events.size(); ++i) {
        m_actionList->insertRow(i);

        auto *numItem = new QTableWidgetItem(QString::number(i + 1));
        numItem->setTextAlignment(Qt::AlignCenter);
        numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
        m_actionList->setItem(i, 0, numItem);

        auto *typeItem = new QTableWidgetItem(eventLabel(events[i]));
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        m_actionList->setItem(i, 1, typeItem);

        auto *delayItem = new QTableWidgetItem(QString::number(events[i].randomDelayMaxMs));
        delayItem->setTextAlignment(Qt::AlignCenter);
        m_actionList->setItem(i, 2, delayItem);

        auto *deleteBtn = new QToolButton();
        deleteBtn->setText(QStringLiteral("✕"));
        const int row = i;
        connect(deleteBtn, &QToolButton::clicked, this, [this, row]() { removeAction(row); });
        m_actionList->setCellWidget(i, 3, deleteBtn);
    }
    m_actionList->blockSignals(false);
}

void MainWindow::onCellChanged(int row, int col) {
    if (col != 2) return;
    QVector<MacroEvent> events = m_recorder->events();
    if (row < 0 || row >= events.size()) return;
    auto *item = m_actionList->item(row, col);
    if (!item) return;
    bool ok = false;
    quint32 val = item->text().trimmed().toUInt(&ok);
    if (!ok) val = 0;
    if (val > 10000) val = 10000;
    events[row].randomDelayMaxMs = val;
    m_recorder->setEvents(events);
    m_actionList->blockSignals(true);
    item->setText(QString::number(val));
    m_actionList->blockSignals(false);
}

void MainWindow::onRunHotkey() {
    if (!m_playing && !m_recorder->isRecording()) startPlayback();
}

void MainWindow::onStopHotkey() {
    if (m_recorder->isRecording()) stopRecording();
    else if (m_playing) stopPlayback();
}

void MainWindow::onRecordHotkey() {
    if (!m_playing) toggleRecording();
}

void MainWindow::onEventRecorded(int totalCount) {
    Q_UNUSED(totalCount)
}

void MainWindow::onPlaybackStarted() {
    m_playing = true;
    updateControls();
}

void MainWindow::onPlaybackFinished() {
    m_playing = false;
    updateControls();
}

void MainWindow::onIterationChanged(int current, int total) {
    Q_UNUSED(current)
    Q_UNUSED(total)
}

void MainWindow::updateControls() {
    const bool recording = m_recorder->isRecording();
    m_recordButton->setText(recording ? tr("Stop Recording") : tr("Record"));
    m_playButton->setText(m_playing ? tr("Stop") : tr("Play"));
    m_recordButton->setEnabled(!m_playing);
    m_playButton->setEnabled(!recording);
    m_clearButton->setEnabled(!recording && !m_playing);
    m_actionList->setEnabled(!recording && !m_playing);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_recorder->isRecording()) m_recorder->stopRecording();
    if (m_player->isRunning()) {
        m_player->requestStop();
        m_player->wait();
    }
    m_recorder->removeHooks();
    event->accept();
}

QString MainWindow::vkName(int vk) {
    if (vk >= 0x70 && vk <= 0x7B)
        return QStringLiteral("F%1").arg(vk - 0x70 + 1);
    return QStringLiteral("0x%1").arg(vk, 0, 16);
}

QString MainWindow::vkKeyName(quint32 vk) {
    switch (vk) {
        case 0x08: return QStringLiteral("Backspace");
        case 0x09: return QStringLiteral("Tab");
        case 0x0D: return QStringLiteral("Enter");
        case 0x10: return QStringLiteral("Shift");
        case 0x11: return QStringLiteral("Ctrl");
        case 0x12: return QStringLiteral("Alt");
        case 0x13: return QStringLiteral("Pause");
        case 0x14: return QStringLiteral("CapsLock");
        case 0x1B: return QStringLiteral("Escape");
        case 0x20: return QStringLiteral("Space");
        case 0x21: return QStringLiteral("Page Up");
        case 0x22: return QStringLiteral("Page Down");
        case 0x23: return QStringLiteral("End");
        case 0x24: return QStringLiteral("Home");
        case 0x25: return QStringLiteral("← Left");
        case 0x26: return QStringLiteral("↑ Up");
        case 0x27: return QStringLiteral("→ Right");
        case 0x28: return QStringLiteral("↓ Down");
        case 0x2D: return QStringLiteral("Insert");
        case 0x2E: return QStringLiteral("Delete");
        case 0x5B: return QStringLiteral("Win");
        case 0x90: return QStringLiteral("NumLock");
        case 0x91: return QStringLiteral("ScrollLock");
        case 0xA0: return QStringLiteral("L-Shift");
        case 0xA1: return QStringLiteral("R-Shift");
        case 0xA2: return QStringLiteral("L-Ctrl");
        case 0xA3: return QStringLiteral("R-Ctrl");
        case 0xA4: return QStringLiteral("L-Alt");
        case 0xA5: return QStringLiteral("R-Alt");
        case 0xBA: return QStringLiteral(";");
        case 0xBB: return QStringLiteral("=");
        case 0xBC: return QStringLiteral(",");
        case 0xBD: return QStringLiteral("-");
        case 0xBE: return QStringLiteral(".");
        case 0xBF: return QStringLiteral("/");
        case 0xC0: return QStringLiteral("`");
        case 0xDB: return QStringLiteral("[");
        case 0xDC: return QStringLiteral("\\");
        case 0xDD: return QStringLiteral("]");
        case 0xDE: return QStringLiteral("'");
        default:
            if (vk >= 0x30 && vk <= 0x39) return QString(QChar(vk));
            if (vk >= 0x41 && vk <= 0x5A) return QString(QChar(vk));
            if (vk >= 0x60 && vk <= 0x69) return QStringLiteral("Num%1").arg(vk - 0x60);
            if (vk >= 0x70 && vk <= 0x7B) return QStringLiteral("F%1").arg(vk - 0x70 + 1);
            return QStringLiteral("0x%1").arg(vk, 2, 16, QChar('0')).toUpper();
    }
}

QString MainWindow::buttonName(MouseButton btn) {
    switch (btn) {
        case MouseButton::Left:   return QStringLiteral("Left");
        case MouseButton::Right:  return QStringLiteral("Right");
        case MouseButton::Middle: return QStringLiteral("Middle");
        default:                  return QStringLiteral("Unknown");
    }
}

QString MainWindow::eventLabel(const MacroEvent &e) {
    switch (e.type) {
        case MacroEventType::MouseMove:  return QStringLiteral("Mouse Move");
        case MacroEventType::MouseDown:  return QStringLiteral("Mouse Down  %1").arg(buttonName(e.button));
        case MacroEventType::MouseUp:    return QStringLiteral("Mouse Up  %1").arg(buttonName(e.button));
        case MacroEventType::MouseWheel: return QStringLiteral("Mouse Wheel  Δ%1").arg(e.wheelDelta);
        case MacroEventType::KeyDown:    return QStringLiteral("Key Down  %1").arg(vkKeyName(e.vkCode));
        case MacroEventType::KeyUp:      return QStringLiteral("Key Up  %1").arg(vkKeyName(e.vkCode));
        case MacroEventType::KeyPress:   return QStringLiteral("Key Press  %1").arg(vkKeyName(e.vkCode));
        default:                          return QString();
    }
}
