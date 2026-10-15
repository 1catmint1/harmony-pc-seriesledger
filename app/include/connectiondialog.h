#pragma once

#include "appmodel.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class ConnectionDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = nullptr);

    void setProfile(const ConnectionProfile &profile);
    ConnectionProfile profile() const;

private:
    QLineEdit *m_name = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox *m_port = nullptr;
    QLineEdit *m_database = nullptr;
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QCheckBox *m_ssl = nullptr;
    QCheckBox *m_ignoreSsl = nullptr;
    int m_id = 0;
};

