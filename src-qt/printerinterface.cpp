
#include "printerinterface.h"
#include <QJsonDocument>


#define GETSTATUSCMD "/home/steve/Development/davpro/davpro -j 192.168.1.7 9100"


PrinterInterface::PrinterInterface()
{
    connect( &proc, SIGNAL(finished(int, QProcess::ExitStatus)),
             this, SLOT(statusupdate(int, QProcess::ExitStatus)) ) ;
    state=INIT ;
    statusstring = "Initialising" ;
    rawdata="" ;
}

void PrinterInterface::setIPAddress(QString ipaddress, int portnum)
{
    if (state!=IDLE) return ;
    confIpAddress = ipaddress ;
    confPortNum = portnum ;
}


void PrinterInterface::updateStatus()
{
    if (state==IDLE || state==INIT) {
        state=GETSTATUS ;
        proc.start(GETSTATUSCMD) ;
    }
}

// When busy or Error, printer only returns status and no further information
// Busy when printer responds but can't give details
bool PrinterInterface::isBusy()
{
    return json["busy"].toBool() ;
}

// Error when unable to connect etc.
bool PrinterInterface::isError()
{
    return (json["error"].toBool()) ;
}

// Printer is idle when waiting for commands
bool PrinterInterface::isIdle()
{
    if (isBusy()) return false ;
    else return json["idle"].toBool() ;
}

// Printer is printing when heating / printing / cooling / pausing etc.
bool PrinterInterface::isPrinting()
{
    if (isBusy()) return false ;
    else return json["printing"].toBool() ;
}


QString PrinterInterface::rawData()
{
    return rawdata ;
}

void PrinterInterface::statusupdate(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus) ;
    Q_UNUSED(exitCode) ;
    QByteArray buffer = proc.readAll() ;
    rawdata = buffer ;
    QJsonParseError handler ;
    QJsonDocument statusdata = QJsonDocument::fromJson(buffer, &handler) ;
    if (!statusdata.isNull()) {
        if (statusdata.isObject()) {
            json = statusdata.object() ;
            statusstring = json.value("status").toString() ;
            if (statusstring.isEmpty()) {
                statusstring = "Error, printer returned no data" ;
            } else {
                statusstring = json.value("status").toString() ;
            }
        } else {
            statusstring = "Error, returned data is not a correctly formatted JSON document object" ;
        }
    } else {
        statusstring = QString("Error in JSON from printer interface: ") + handler.errorString() ;
    }
    proc.close() ;
    state=IDLE ;

    emit(statusupdated()) ;
}

QString PrinterInterface::serialNumber() {
    QJsonValue o = json.value("serial") ;
    QString s = o.toString() ;
    return s ;
}

QString PrinterInterface::modelNumber() { return json.value("model").toString() ;}

int PrinterInterface::nozzles() { return json["nozzles"].toInt() ;}

float PrinterInterface::filamentRemaining(int nozzlenumber) {
    QJsonObject filament = json["filamentremaining"].toObject() ;
    double remaining = filament[QString::number(nozzlenumber+1)].toDouble() ;
    return (float)remaining ;
}

float PrinterInterface::nozzleTemperature(int nozzlenumber) {
    QJsonObject nozzle = json["nozzletemperature"].toObject() ;
    double temperature = nozzle[QString::number(nozzlenumber+1)].toDouble() ;
    return (float)temperature ;
}

float PrinterInterface::bedTemperature() {return (float)json["bedtemperature"].toDouble() ;}

QString PrinterInterface::status() { return statusstring ; }

QString PrinterInterface::ipAddress() { return json["ipaddress"].toString() ;}

bool PrinterInterface::isDoorOpen()
{
    return json["dooropen"].toBool() ;
}

bool PrinterInterface::isLidOpen()
{
    return json["lidopen"].toBool() ;
}

int PrinterInterface::percentComplete()
{
    return json["percentcomplete"].toInt() ;
}

float PrinterInterface::remainingMinutes()
{
    return (float)json["remainingmins"].toDouble() ;
}

float PrinterInterface::elapsedMinutes()
{
    return (float)json["elapsedmins"].toDouble() ;
}
