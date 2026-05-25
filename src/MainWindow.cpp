#include "MainWindow.h"

#include "MacroRecorder.h"
#include "MacroPlayer.h"
#include "SettingsDialog.h"
#include "LicenseDialog.h"
#include "MacroEvent.h"

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
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
#include <QStringList>
#include <QString>

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
    setStatus(tr("Idle"));
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

    m_recordButton = new QPushButton(tr("Record"), central);
    m_playButton = new QPushButton(tr("Start / Stop"), central);
    m_saveButton = new QPushButton(tr("Save"), central);
    m_loadButton = new QPushButton(tr("Load"), central);
    m_settingsButton = new QPushButton(tr("Settings"), central);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addWidget(m_recordButton);
    buttonRow->addWidget(m_playButton);
    buttonRow->addWidget(m_saveButton);
    buttonRow->addWidget(m_loadButton);
    buttonRow->addWidget(m_settingsButton);

    m_statusLabel = new QLabel(tr("Idle"), central);
    m_eventCountLabel = new QLabel(tr("Events: 0"), central);
    m_hotkeyLabel = new QLabel(central);

    auto *statusGroup = new QGroupBox(tr("Status"), central);
    auto *statusLayout = new QVBoxLayout(statusGroup);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addWidget(m_eventCountLabel);
    statusLayout->addWidget(m_hotkeyLabel);

    auto *layout = new QVBoxLayout(central);
    layout->addLayout(buttonRow);
    layout->addWidget(statusGroup);
    layout->addStretch(1);

    setCentralWidget(central);

    connect(m_recordButton, &QPushButton::clicked, this, &MainWindow::toggleRecording);
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveMacro);
    connect(m_loadButton, &QPushButton::clicked, this, &MainWindow::loadMacro);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);

    resize(520, 240);
}

void MainWindow::buildMenu() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Save..."), this, &MainWindow::saveMacro);
    fileMenu->addAction(tr("&Load..."), this, &MainWindow::loadMacro);
    fileMenu->addAction(tr("Se&ttings..."), this, &MainWindow::openSettings);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&License"), this, &MainWindow::showLicense);
    helpMenu->addAction(tr("&About"), this, &MainWindow::showAbout);
}

void MainWindow::applyHotkeys() {
    m_recorder->setHotkeys(m_settings.runHotkeyVk(), m_settings.stopHotkeyVk(), m_settings.recordHotkeyVk());
    const QString text = tr("Hotkeys - Run: %1   Stop: %2   Record: %3")
                             .arg(vkName(m_settings.runHotkeyVk()))
                             .arg(vkName(m_settings.stopHotkeyVk()))
                             .arg(vkName(m_settings.recordHotkeyVk()));
    if (m_hotkeyLabel != nullptr) {
        m_hotkeyLabel->setText(text);
    }
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
    if (m_playing || m_recorder->isRecording()) {
        return;
    }
    m_recorder->startRecording();
    onEventRecorded(0);
    setStatus(tr("Recording..."));
    updateControls();
}

void MainWindow::stopRecording() {
    if (!m_recorder->isRecording()) {
        return;
    }
    m_recorder->stopRecording();
    setStatus(tr("Recorded %1 events").arg(m_recorder->eventCount()));
    updateControls();
}

void MainWindow::startPlayback() {
    if (m_playing || m_recorder->isRecording() || m_player->isRunning()) {
        return;
    }
    if (m_recorder->eventCount() == 0) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Nothing to play. Record or load a macro first."));
        return;
    }
    m_player->configure(m_recorder->events(), m_settings.repeatCount(), m_settings.repeatInfinite());
    m_player->start();
}

void MainWindow::stopPlayback() {
    if (!m_playing) {
        return;
    }
    m_player->requestStop();
}

void MainWindow::saveMacro() {
    if (m_recorder->isRecording()) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Stop recording before saving."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Macro"), QString(),
                                                      tr("EasyMacro Files (*.emacro);;JSON Files (*.json)"));
    if (path.isEmpty()) {
        return;
    }

    QJsonArray array;
    for (const MacroEvent &e : m_recorder->events()) {
        array.append(e.toJson());
    }
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
    setStatus(tr("Saved %1 events").arg(m_recorder->eventCount()));
}

void MainWindow::loadMacro() {
    if (m_recorder->isRecording() || m_playing) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Stop the current action before loading."));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Load Macro"), QString(),
                                                      tr("EasyMacro Files (*.emacro);;JSON Files (*.json)"));
    if (path.isEmpty()) {
        return;
    }

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
        if (value.isObject()) {
            events.append(MacroEvent::fromJson(value.toObject()));
        }
    }
    m_recorder->setEvents(events);
    onEventRecorded(events.size());
    setStatus(tr("Loaded %1 events").arg(events.size()));
    updateControls();
}

void MainWindow::openSettings() {
    if (m_recorder->isRecording() || m_playing) {
        QMessageBox::information(this, tr("EasyMacro"), tr("Stop the current action before changing settings."));
        return;
    }
    SettingsDialog dialog(m_settings, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_settings.setRunHotkeyVk(dialog.runHotkeyVk());
    m_settings.setStopHotkeyVk(dialog.stopHotkeyVk());
    m_settings.setRecordHotkeyVk(dialog.recordHotkeyVk());
    m_settings.setRepeatCount(dialog.repeatCount());
    m_settings.setRepeatInfinite(dialog.repeatInfinite());
    m_settings.save();
    applyHotkeys();
}

void MainWindow::showLicense() {
    LicenseDialog dialog(this);
    dialog.exec();
}

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About EasyMacro"),
                       tr("EasyMacro\n\nA simple mouse and keyboard macro recorder.\n"
                          "Records your input and replays it on demand."));
}

void MainWindow::onRunHotkey() {
    if (!m_playing && !m_recorder->isRecording()) {
        startPlayback();
    }
}

void MainWindow::onStopHotkey() {
    if (m_recorder->isRecording()) {
        stopRecording();
    } else if (m_playing) {
        stopPlayback();
    }
}

void MainWindow::onRecordHotkey() {
    if (m_playing) {
        return;
    }
    toggleRecording();
}

void MainWindow::onEventRecorded(int totalCount) {
    m_eventCountLabel->setText(tr("Events: %1").arg(totalCount));
}

void MainWindow::onPlaybackStarted() {
    m_playing = true;
    setStatus(tr("Playing..."));
    updateControls();
}

void MainWindow::onPlaybackFinished() {
    m_playing = false;
    setStatus(tr("Playback finished"));
    updateControls();
}

void MainWindow::onIterationChanged(int current, int total) {
    if (total == 0) {
        setStatus(tr("Playing... loop %1 (infinite)").arg(current));
    } else {
        setStatus(tr("Playing... loop %1 / %2").arg(current).arg(total));
    }
}

void MainWindow::updateControls() {
    const bool recording = m_recorder->isRecording();
    m_recordButton->setText(recording ? tr("Stop Recording") : tr("Record"));
    m_playButton->setText(m_playing ? tr("Stop") : tr("Start"));

    m_recordButton->setEnabled(!m_playing);
    m_playButton->setEnabled(!recording);
    m_saveButton->setEnabled(!recording && !m_playing);
    m_loadButton->setEnabled(!recording && !m_playing);
    m_settingsButton->setEnabled(!recording && !m_playing);
}

void MainWindow::setStatus(const QString &text) {
    if (m_statusLabel != nullptr) {
        m_statusLabel->setText(text);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_recorder->isRecording()) {
        m_recorder->stopRecording();
    }
    if (m_player->isRunning()) {
        m_player->requestStop();
        m_player->wait();
    }
    m_recorder->removeHooks();
    event->accept();
}

QString MainWindow::vkName(int vk) {
    if (vk >= 0x70 && vk <= 0x7B) {
        return QStringLiteral("F%1").arg(vk - 0x70 + 1);
    }
    return QStringLiteral("0x%1").arg(vk, 0, 16);
}
