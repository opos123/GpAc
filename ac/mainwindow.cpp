#include "mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QObject *parent) : QObject(parent),
    m_yaw(0.0), m_pitch(0.0), m_roll(0.0), m_latitude(0.0), m_longitude(0.0)
{
    // Configure the serial port
    m_serial.setPortName("COM5"); // Sesuaikan nama port sesuai kebutuhan
    m_serial.setBaudRate(QSerialPort::Baud9600);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    // Connect the readyRead signal to our readData slot
    connect(&m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    // Open the serial port
    if (!m_serial.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open serial port!";
    }
}

void MainWindow::readData()
{
    while (m_serial.canReadLine()) {
        QByteArray line = m_serial.readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            m_yaw = obj["yaw"].toDouble();
            m_pitch = obj["pitch"].toDouble(); // Tambahkan membaca nilai pitch
            m_roll = obj["roll"].toDouble();   // Tambahkan membaca nilai roll
            m_latitude = obj["latitude"].toDouble();
            m_longitude = obj["longitude"].toDouble();
            emit yawChanged();
            emit pitchChanged();   // Emit signal ketika nilai pitch berubah
            emit rollChanged();    // Emit signal ketika nilai roll berubah
            emit latitudeChanged();
            emit longitudeChanged();
        }
    }
}
