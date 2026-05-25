#ifndef EASYMACRO_LICENSEDIALOG_H
#define EASYMACRO_LICENSEDIALOG_H

#include <QDialog>

class LicenseDialog : public QDialog {
    Q_OBJECT
public:
    explicit LicenseDialog(QWidget *parent = nullptr);
};

#endif
