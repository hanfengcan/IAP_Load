#include "widget.h"
#include "ui_widget.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QIntValidator>
#include <QLineEdit>
#include <QFileDialog>
#include <QDataStream>
#include <QFileInfo>
#include <stdio.h>

//#include <QDebug>

QT_USE_NAMESPACE

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    serial = new QSerialPort(this);
    OpenStatus = false;
    intValidator = new QIntValidator(0, 4000000, this);
    fileLocation = "../";
    binLoadCnt = 1;
    usart_temp = 0;

    ui->baundRateBox->setInsertPolicy (QComboBox::NoInsert);
    ui->tabWidget->setCurrentIndex (2);
    ui->loadButton->setEnabled (false);
    ui->viewButton->setEnabled (false);

    connect (ui->baundRateBox, SIGNAL(currentIndexChanged(int)),
             this, SLOT(checkCustomBaudRate(int)));

    connect (serial, SIGNAL(readyRead()), this, SLOT(readData()));

    connect (this, SIGNAL(transmitData(int)), this, SLOT(transmitDataFun(int)));
    connect (this, SIGNAL(binFileOpenFailed()), this, SLOT(on_fileButton_clicked()));

    fillPortsInfo();
    fillPortsCom();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::fillPortsInfo ()
{
    //添加波特率
    ui->baundRateBox->addItem (QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->baundRateBox->addItem (QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->baundRateBox->addItem (QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->baundRateBox->addItem (QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->baundRateBox->addItem (QStringLiteral("Custom"));
    ui->baundRateBox->setCurrentIndex (3);      //默认115200
    //添加数据位
    ui->dataBitBox->addItem (QStringLiteral("7"), QSerialPort::Data7);
    ui->dataBitBox->addItem (QStringLiteral("8"), QSerialPort::Data8);
    ui->dataBitBox->setCurrentIndex (1);        //默认8
    //添加校验方式
    ui->parityBox->addItem (QStringLiteral("None"), QSerialPort::NoParity);
    ui->parityBox->addItem (QStringLiteral("偶校验"), QSerialPort::EvenParity);
    ui->parityBox->addItem (QStringLiteral("奇校验"), QSerialPort::OddParity);
    ui->parityBox->addItem (QStringLiteral("Mark"), QSerialPort::MarkParity);
    ui->parityBox->addItem (QStringLiteral("Space"), QSerialPort::SpaceParity);
    //添加数据位
    ui->stopBitBox->addItem (QStringLiteral("1"), QSerialPort::OneStop);
    ui->stopBitBox->addItem (QStringLiteral("2"), QSerialPort::TwoStop);

}

void Widget::fillPortsCom ()
{
    ui->uNameBox->clear ();
    static const QString blankString = QObject::tr("N/A");
    QString description;
    QString manufacturer;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QStringList list;
        description = info.description();
        manufacturer = info.manufacturer();
        list << info.portName()
             << (!description.isEmpty () ? description : blankString)
             << (!manufacturer.isEmpty () ? manufacturer : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number (info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number (info.productIdentifier(), 16) : blankString);
        ui->uNameBox->addItem (list.first (), list);
    }
}

//槽函数

void Widget::checkCustomBaudRate (int idx)
{
    bool isCustomBaudRate = !ui->baundRateBox->itemData (idx).isValid ();
    ui->baundRateBox->setEditable (isCustomBaudRate);
    if (isCustomBaudRate) {
        ui->baundRateBox->clearEditText ();
        QLineEdit *edit = ui->baundRateBox->lineEdit ();
        edit->setValidator (intValidator);
    }
}

void Widget::readData ()
{

    usart_temp += serial->readAll ();       //存储收到数据
    if(usart_temp == "Next\n")
    {
        emit transmitData(binLoadCnt++);        //发送写入完成信号

        ui->loadTextEdit->insertPlainText (tr("成功写入1K,总共写入%1K\n").arg (binLoadCnt));
        usart_temp.clear ();
    }
    if(usart_temp == "OK\n")
    {
        ui->loadTextEdit->insertPlainText (tr("IAP结束\n"));
        usart_temp.clear ();
    }
    if(usart_temp == "again\n")
    {
        emit transmitData (--binLoadCnt);
        ui->loadTextEdit->insertPlainText (tr("未成功写入，尝试重新写入\n"));
        usart_temp.clear ();
    }

}

void Widget::transmitDataFun (int cnt)             //发送文件
{
    qint16 temp = 0;            //剩余待传数据

    ui->loadButton->setEnabled (false);
    QFile *binFile = new QFile(fileLocation);
    binFile->open (QIODevice::ReadOnly);
    binFile->seek (cnt * 1024);


    char * binByte = new char[1024];         //bin缓存
    memset (binByte, 0xff, 1024);

    //data_len_L  data_len_H  data(no more than 1K)   index_L index_H CRC

    QByteArray binTransmit;
    binTransmit.resize (8);
    temp = binSize - 1024*cnt;
    if(temp < 1024)
    {

        binTransmit[0] = temp % 256;
        binTransmit[1] = temp / 256;
        //index CRC 暂空
        binTransmit[2] = cnt % 256;
        binTransmit[3] = cnt / 256;

        binTransmit[4] = 0xff;
        binTransmit[5] = 0xff;
        binTransmit[6] = 0xff;
        binTransmit[7] = 0xff;

        binFile->read (binByte, temp);
        binTransmit.insert(2, binByte, 1024);

        crc.crc32 (&binTransmit, binTransmit.size () - 4);
        unsigned long crc_val = crc.getCrc32 ();

        binTransmit[1024 + 4] = crc_val;
        binTransmit[1024 + 5] = crc_val >> 8;
        binTransmit[1024 + 6] = crc_val >> 16;
        binTransmit[1024 + 7] = crc_val >> 24;

        ui->loadProgressBar->setValue (100);
    }
    else
    {

        binTransmit[0] = 0x00;
        binTransmit[1] = 0x04;
        //index CRC 暂空
        binTransmit[2] = cnt % 256;     //L
        binTransmit[3] = cnt / 256;     //H
        binTransmit[4] = 0xff;          //L
        binTransmit[5] = 0xff;
        binTransmit[6] = 0xff;
        binTransmit[7] = 0xff;          //H

        binFile->read (binByte, 1024);
        binTransmit.insert(2, binByte, 1024);

        crc.crc32 (&binTransmit, binTransmit.size () - 4);
        unsigned long crc_val = crc.getCrc32 ();

        binTransmit[1024 + 4] = crc_val;
        binTransmit[1024 + 5] = crc_val >> 8;
        binTransmit[1024 + 6] = crc_val >> 16;
        binTransmit[1024 + 7] = crc_val >> 24;


        int i = (1 - float(temp) / float(binSize)) * 100;
        ui->loadProgressBar->setValue (i);
    }
    delete binByte;
    serial->write(binTransmit);
    serial->write ("\n");       //用来触发STM32串口中断,置位0x8000
}

void Widget::on_openButton_clicked()
{
    if(!OpenStatus)
    {
        currentSettings.name = ui->uNameBox->currentText ();
        if (ui->baundRateBox->currentIndex () == 4)
        {
            currentSettings.baudRate = ui->baundRateBox->currentText ().toInt ();
        }
        else
        {
            currentSettings.baudRate = static_cast<QSerialPort::BaudRate>
                    (ui->baundRateBox->itemData (ui->baundRateBox->currentIndex()).toInt ());
        }
        currentSettings.dataBits = static_cast<QSerialPort::DataBits>
                (ui->dataBitBox->itemData (ui->dataBitBox->currentIndex ()).toInt ());
        currentSettings.stopBits = static_cast<QSerialPort::StopBits>
                (ui->stopBitBox->itemData (ui->stopBitBox->currentIndex ()).toInt ());
        currentSettings.parity = static_cast<QSerialPort::Parity>
                (ui->parityBox->itemData (ui->parityBox->currentIndex ()).toInt ());

        serial->setPortName (currentSettings.name);
        if(serial->open (QIODevice::ReadWrite))
        {
            if( serial->setBaudRate (currentSettings.baudRate)
                && serial->setDataBits (currentSettings.dataBits)
                && serial->setStopBits (currentSettings.stopBits)
                && serial->setParity (currentSettings.parity) )
            {
                OpenStatus = true;
                serial->write (":updata;");
                ui->loadButton->setEnabled (true);
                ui->openButton->setText (tr("关闭串口"));
            }
            else
            {
                serial->close ();
                ui->loadButton->setEnabled (false);
                OpenStatus = false;
                ui->openButton->setText (tr("打开串口"));
            }
        }
        else {
            OpenStatus = false;
            ui->loadButton->setEnabled (false);
            ui->openButton->setText (tr("打开串口"));
        }
    }
    else
    {
        OpenStatus = false;
        ui->loadButton->setEnabled (false);
        ui->openButton->setText (tr("打开串口"));
        serial->close ();
    }
}

void Widget::on_fileButton_clicked()        //打开文件
{
    fileLocation = QFileDialog::getOpenFileName (this, tr("Open File"),
                                                 fileLocation, tr("文件 (*.bin)"));
    ui->fileEdit->setText (fileLocation);
    if(fileLocation != NULL)
    {
        QFileInfo *temp = new QFileInfo(fileLocation);
        binSize = temp->size ();
        if(binSize < 1024)
            ui->sizeLabel->setText (tr("Size: %1 B").arg (binSize));
        else if(binSize < 1024 * 1024)
            ui->sizeLabel->setText (tr("Size: %1 K %2 B").arg (binSize / 1024).arg (binSize % 1024));
        else
            ui->sizeLabel->setText (tr("Size: %1 M %2 K").arg (binSize / 1024 / 1024)
                                    .arg (binSize / 1024 %1024));

        QFile *binFile = new QFile(fileLocation);
        if(!binFile->open (QIODevice::ReadOnly))emit binFileOpenFailed ();  //重新打开文件
        else
        {
            ui->loadButton->setEnabled (true);
            ui->viewButton->setEnabled (true);
            binFile->close ();
        }
        delete binFile;

        ui->binTextEdit->clear ();
        delete temp;
    }
}

void Widget::on_loadButton_clicked()
{

    ui->tabWidget->setCurrentIndex (0);
    ui->loadTextEdit->insertPlainText (tr("正在下载\n"));
    binLoadCnt = 1;         //初始化 文件偏移值 binLoadCnt*1024
    usart_temp.clear ();
    emit transmitData (0);      //发送首K内容

}

void Widget::on_viewButton_clicked()            //查看bin内容
{


    QFile *binFile = new QFile(fileLocation);
    if(!binFile->open (QIODevice::ReadOnly))emit binFileOpenFailed ();  //重新打开文件
    else ui->tabWidget->setCurrentIndex (1);

    QDataStream in(binFile);
    char * binByte = new char[binSize];
    in.setVersion (QDataStream::Qt_4_0);

    in.readRawData (binByte, binSize);      //读出文件到缓存

    QByteArray *tempByte = new QByteArray(binByte, binSize);                //格式转换
    delete[] binByte;
    QString *tempStr = new QString(tempByte->toHex ().toUpper ());
    delete tempByte;
    //显示文件内容
    qint8 cnt = 1;
    qint16 kcnt = 0;
    for(qint64 i = 2; i < tempStr->size ();)
    {
        tempStr->insert (i, ' ');
        i += 3;
        cnt++;
        if(cnt == 8)
        {
            tempStr->insert (i, ' ');
            i += 1;
        }
        if(cnt == 16)
        {
            cnt = 1;
            kcnt ++;
            if(kcnt == 64)
            {
                kcnt = 0;
                tempStr->insert (i, '\n');
                i++;
            }
            tempStr->insert (i, '\n');
            i += 3;         //避免换行后开头一个空格
        }

    }
    ui->binTextEdit->insertPlainText (*tempStr);

    delete tempStr;

    binFile->close ();
    delete binFile;
}

void Widget::on_refreshButton_clicked()
{
    fillPortsCom();
}
