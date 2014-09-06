#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include <QDialog>
#include <QtSerialPort/QSerialPort>
#include <QFile>

#include <crc_cal.h>

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

namespace Ui {
class Widget;
}

class   QIntValidator;

QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    struct Settings {
        QString name;
        qint32  baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity   parity;           //校验
        QSerialPort::StopBits stopBits;

    };

    explicit Widget(QWidget *parent = 0);
    ~Widget();

    void fillPortsInfo();
    void fillPortsCom();

private slots:
    void checkCustomBaudRate(int idx);
    void readData(void);
    void transmitDataFun(int cnt);             //发送文件

    void on_openButton_clicked();

    void on_fileButton_clicked();

    void on_loadButton_clicked();

    void on_viewButton_clicked();

    void on_refreshButton_clicked();

signals:
    void transmitData(int);
    void binFileOpenFailed(void);

private:
    Ui::Widget *ui;
    Settings currentSettings;
    QIntValidator *intValidator;

    QSerialPort *serial;
    bool OpenStatus;

    QString fileLocation;
    quint64 binSize;        //文件大小
    QFile *binFile;

    QByteArray usart_temp;
    qint16 binLoadCnt;  //记录下载了多少K

    CRC_Cal crc;
};

#endif // WIDGET_H
