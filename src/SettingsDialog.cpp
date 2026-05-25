#include "SettingsDialog.h"

#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QVector>
#include <QPair>
#include <QString>

namespace {

struct HotkeyOption {
    const char *name;
    int vk;
};

const QVector<HotkeyOption> &hotkeyOptions() {
    static const QVector<HotkeyOption> options = {
        {"F1", 0x70}, {"F2", 0x71}, {"F3", 0x72}, {"F4", 0x73},
        {"F5", 0x74}, {"F6", 0x75}, {"F7", 0x76}, {"F8", 0x77},
        {"F9", 0x78}, {"F10", 0x79}, {"F11", 0x7A}, {"F12", 0x7B}
    };
    return options;
}

}

SettingsDialog::SettingsDialog(const Settings &current, QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Settings"));
    setModal(true);

    m_runCombo = new QComboBox(this);
    m_stopCombo = new QComboBox(this);
    m_recordCombo = new QComboBox(this);
    populateHotkeyCombo(m_runCombo, current.runHotkeyVk());
    populateHotkeyCombo(m_stopCombo, current.stopHotkeyVk());
    populateHotkeyCombo(m_recordCombo, current.recordHotkeyVk());

    auto *hotkeyGroup = new QGroupBox(tr("Hotkeys"), this);
    auto *hotkeyForm = new QFormLayout(hotkeyGroup);
    hotkeyForm->addRow(tr("Run macro:"), m_runCombo);
    hotkeyForm->addRow(tr("Stop:"), m_stopCombo);
    hotkeyForm->addRow(tr("Record:"), m_recordCombo);

    m_repeatSpin = new QSpinBox(this);
    m_repeatSpin->setRange(1, 1000000);
    m_repeatSpin->setValue(current.repeatCount());

    m_infiniteCheck = new QCheckBox(tr("Repeat infinitely"), this);
    m_infiniteCheck->setChecked(current.repeatInfinite());

    auto *repeatGroup = new QGroupBox(tr("Playback"), this);
    auto *repeatForm = new QFormLayout(repeatGroup);
    repeatForm->addRow(tr("Repeat count:"), m_repeatSpin);
    repeatForm->addRow(QString(), m_infiniteCheck);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(hotkeyGroup);
    layout->addWidget(repeatGroup);
    layout->addWidget(buttons);

    connect(m_infiniteCheck, &QCheckBox::toggled, this, &SettingsDialog::onInfiniteToggled);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

    onInfiniteToggled(m_infiniteCheck->isChecked());
}

void SettingsDialog::populateHotkeyCombo(QComboBox *combo, int selectedVk) {
    const QVector<HotkeyOption> &options = hotkeyOptions();
    for (const HotkeyOption &option : options) {
        combo->addItem(QString::fromLatin1(option.name), option.vk);
    }
    const int index = combo->findData(selectedVk);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

int SettingsDialog::vkFromCombo(const QComboBox *combo) {
    return combo->currentData().toInt();
}

void SettingsDialog::onInfiniteToggled(bool checked) {
    m_repeatSpin->setEnabled(!checked);
}

void SettingsDialog::onAccept() {
    const int runVk = vkFromCombo(m_runCombo);
    const int stopVk = vkFromCombo(m_stopCombo);
    const int recordVk = vkFromCombo(m_recordCombo);
    if (runVk == stopVk || runVk == recordVk || stopVk == recordVk) {
        QMessageBox::warning(this, tr("Invalid hotkeys"),
                             tr("Run, Stop and Record hotkeys must all be different."));
        return;
    }
    accept();
}

int SettingsDialog::runHotkeyVk() const {
    return vkFromCombo(m_runCombo);
}

int SettingsDialog::stopHotkeyVk() const {
    return vkFromCombo(m_stopCombo);
}

int SettingsDialog::recordHotkeyVk() const {
    return vkFromCombo(m_recordCombo);
}

int SettingsDialog::repeatCount() const {
    return m_repeatSpin->value();
}

bool SettingsDialog::repeatInfinite() const {
    return m_infiniteCheck->isChecked();
}
