#ifndef EASYMACRO_MAINWINDOW_H
#define EASYMACRO_MAINWINDOW_H

#include <QMainWindow>
#include "Settings.h"

class QPushButton;
class QLabel;
class QCloseEvent;
class MacroRecorder;
class MacroPlayer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void toggleRecording();
    void togglePlayback();
    void saveMacro();
    void loadMacro();
    void openSettings();
    void showLicense();
    void showAbout();

    void onRunHotkey();
    void onStopHotkey();
    void onRecordHotkey();

    void onEventRecorded(int totalCount);
    void onPlaybackStarted();
    void onPlaybackFinished();
    void onIterationChanged(int current, int total);

private:
    void buildUi();
    void buildMenu();
    void applyHotkeys();
    void startRecording();
    void stopRecording();
    void startPlayback();
    void stopPlayback();
    void updateControls();
    void setStatus(const QString &text);
    static QString vkName(int vk);

    Settings m_settings;
    MacroRecorder *m_recorder = nullptr;
    MacroPlayer *m_player = nullptr;

    QPushButton *m_recordButton = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_loadButton = nullptr;
    QPushButton *m_settingsButton = nullptr;

    QLabel *m_statusLabel = nullptr;
    QLabel *m_eventCountLabel = nullptr;
    QLabel *m_hotkeyLabel = nullptr;

    bool m_playing = false;
};

#endif
