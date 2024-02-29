#include "to_encrypted.h"

#include <QHBoxLayout>
#include <QMessageBox>

to_encrypted::~to_encrypted()
{
}

to_encrypted::to_encrypted(QWidget *parent)
    : QDialog(parent)
{
    m_keyLabel = new QLabel(this);
    m_keyLabel->setText("密钥：");
    m_keyEdit = new QLineEdit(this);
    m_timeLabel = new QLabel(this);
    m_timeLabel->setText("时间：");
    m_timeEdit = new QLineEdit(this);
    m_encryptButton = new QPushButton(this);
    m_encryptButton->setText("加密");

    QHBoxLayout *hLayout1 = new QHBoxLayout;
    hLayout1->addWidget(m_keyLabel);
    hLayout1->addWidget(m_keyEdit);
    QHBoxLayout *hLayout2 = new QHBoxLayout;
    hLayout2->addWidget(m_timeLabel);
    hLayout2->addWidget(m_timeEdit);
    QHBoxLayout *hLayout3 = new QHBoxLayout;
    hLayout3->addWidget(m_encryptButton);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hLayout1);
    vLayout->addLayout(hLayout2);
    vLayout->addLayout(hLayout3);

    setLayout(vLayout);

    connect(m_encryptButton, &QPushButton::clicked, this, &to_encrypted::onEncryptButtonClicked);
}

void to_encrypted::onEncryptButtonClicked()
{
    QString key = m_keyEdit->text();
    QString time = m_timeEdit->text();

    if (key.isEmpty() || time.isEmpty()) {
        QMessageBox::warning(this, "错误", "密钥或时间不能为空！");
        return;
    }

    // 加密数据
   QString encryptedData = encryptData(key, time);

   // 显示加密数据
   QMessageBox::information(this, "加密结果", encryptedData);
}
