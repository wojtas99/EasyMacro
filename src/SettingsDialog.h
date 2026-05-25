#ifndef EASYMACRO_SETTINGSDIALOG_H
#define EASYMACRO_SETTINGSDIALOG_H

#include <QDialog>
#include "Settings.h"

class QComboBox;
class QSpinBox;
class QCheckBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const Settings &current, QWidget *parent = nullptr);

    int runHotkeyVk() const;
    int stopHotkeyVk() const;
    int recordHotkeyVk() const;
    int repeatCount() const;
    bool repeatInfinite() const;
    bool instantSpeed() const;

private slots:
    void onInfiniteToggled(bool checked);
    void onAccept();

private:
    void populateHotkeyCombo(QComboBox *combo, int selectedVk);
    static int vkFromCombo(const QComboBox *combo);

    QComboBox *m_runCombo = nullptr;
    QComboBox *m_stopCombo = nullptr;
    QComboBox *m_recordCombo = nullptr;
    QSpinBox *m_repeatSpin = nullptr;
    QCheckBox *m_infiniteCheck = nullptr;
    QComboBox *m_speedCombo = nullptr;
};

#endif
