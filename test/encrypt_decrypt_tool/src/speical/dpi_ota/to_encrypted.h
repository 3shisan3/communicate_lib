#ifndef TO_ENCRYPTED_H
#define TO_ENCRYPTED_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class to_encrypted : public QDialog
{
    Q_OBJECT

public:
    to_encrypted(QWidget *parent = nullptr);
    ~to_encrypted();

private:
    QLabel *m_keyLabel;
    QLineEdit *m_keyEdit;
    QLabel *m_timeLabel;
    QLineEdit *m_timeEdit;
    QPushButton *m_encryptButton;

private slots:
    void onEncryptButtonClicked();
};
#endif // TO_ENCRYPTED_H
