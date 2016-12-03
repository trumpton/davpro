#ifndef PRINTERINTERFACE_H
#define PRINTERINTERFACE_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>

#define IDLE 0
#define INIT 1
#define GETSTATUS 2
#define UPLOADFILE 3

class PrinterInterface : public QObject
{
    Q_OBJECT

public:
    PrinterInterface();
    bool isBusy() ;
    bool isPrinting() ;
    bool isIdle() ;
    bool isError() ;

    QString rawData() ;
    void setIPAddress(QString ipaddress, int portnum) ;
    void updateStatus() ;
    QString serialNumber() ;
    QString modelNumber() ;
    int nozzles() ;
    float filamentRemaining(int nozzlenumber=0) ;
    float nozzleTemperature(int nozzlenumber=0) ;
    float bedTemperature() ;
    int percentComplete() ;
    float remainingMinutes() ;
    float elapsedMinutes() ;

    QString status() ;
    QString ipAddress() ;
    bool isDoorOpen() ;
    bool isLidOpen() ;

private:
    QString statusstring ;
    QString confIpAddress ;
    int confPortNum ;
    QJsonObject json ;
    QProcess proc ;
    int state ;
    QString rawdata ;

private slots:
    void statusupdate(int exitCode, QProcess::ExitStatus exitStatus) ;

signals:
    void statusupdated() ;

};

#endif // PRINTERINTERFACE_H


