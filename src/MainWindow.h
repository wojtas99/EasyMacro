#ifndef EASYMACRO_MAINWINDOW_H
#define EASYMACRO_MAINWINDOW_H

#include <QMainWindow>
#include "Settings.h"
#include "MacroEvent.h"

class QPushButton;
class QTableWidget;
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
    void removeAction(int row);
    void onCellChanged(int row, int col);

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
    void updateActionList();
    static QString vkName(int vk);
    static QString eventLabel(const MacroEvent &e);
    static QString buttonName(MouseButton btn);
    static QString vkKeyName(quint32 vk);

    Settings m_settings;
    MacroRecorder *m_recorder = nullptr;
    MacroPlayer *m_player = nullptr;

    QPushButton *m_recordButton = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QTableWidget *m_actionList = nullptr;

    bool m_playing = false;
};

#endif
