#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QSerialPort>
#include <QJsonDocument>
#include <QJsonObject>

class MainWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double yaw READ yaw NOTIFY yawChanged)
    Q_PROPERTY(double pitch READ pitch NOTIFY pitchChanged)
    Q_PROPERTY(double roll READ roll NOTIFY rollChanged)
    Q_PROPERTY(double latitude READ latitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude NOTIFY longitudeChanged)

public:
    explicit MainWindow(QObject *parent = nullptr);

    double yaw() const { return m_yaw; }
    double pitch() const { return m_pitch; }
    double roll() const { return m_roll; }
    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }

signals:
    void yawChanged();
    void pitchChanged();
    void rollChanged();
    void latitudeChanged();
    void longitudeChanged();

private slots:
    void readData();

private:
    QSerialPort m_serial;
    double m_yaw;
    double m_pitch;
    double m_roll;
    double m_latitude;
    double m_longitude;
};

#endif // MAINWINDOW_H
