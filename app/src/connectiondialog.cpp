#include "connectiondialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

ConnectionDialog::ConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("InfluxDB 连接"));
    setModal(true);
    resize(480, 360);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_name = new QLineEdit(this);
    m_host = new QLineEdit(this);
    m_port = new QSpinBox(this);
    m_port->setRange(1, 65535);
    m_database = new QLineEdit(this);
    m_username = new QLineEdit(this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_ssl = new QCheckBox(tr("使用 HTTPS"), this);
    m_ignoreSsl = new QCheckBox(tr("忽略自签名证书错误"), this);
    m_ignoreSsl->setToolTip(tr("仅在明确知道服务端证书可信时启用。"));

    form->addRow(tr("名称"), m_name);
    form->addRow(tr("主机"), m_host);
    form->addRow(tr("端口"), m_port);
    form->addRow(tr("默认数据库"), m_database);
    form->addRow(tr("用户名"), m_username);
    form->addRow(tr("密码"), m_password);
    form->addRow(QString(), m_ssl);
    form->addRow(QString(), m_ignoreSsl);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    ConnectionProfile defaults;
    setProfile(defaults);
}

void ConnectionDialog::setProfile(const ConnectionProfile &profile)
{
    m_id = profile.id;
    m_name->setText(profile.name);
    m_host->setText(profile.host);
    m_port->setValue(profile.port);
    m_database->setText(profile.database);
    m_username->setText(profile.username);
    m_password->setText(profile.password);
    m_ssl->setChecked(profile.useSsl);
    m_ignoreSsl->setChecked(profile.ignoreSslErrors);
}

ConnectionProfile ConnectionDialog::profile() const
{
    ConnectionProfile value;
    value.id = m_id;
    value.name = m_name->text().trimmed();
    value.host = m_host->text().trimmed();
    value.port = m_port->value();
    value.database = m_database->text().trimmed();
    value.username = m_username->text();
    value.password = m_password->text();
    value.useSsl = m_ssl->isChecked();
    value.ignoreSslErrors = m_ignoreSsl->isChecked();
    return value;
}

