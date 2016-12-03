#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "QTimer"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->frameNozzle2->hide() ;
    ui->progressBar->hide() ;

    // Setup printer interface
    // printer.setIPAddress(QString("192.168.1.7"), 9100) ;
    connect(&printer, SIGNAL(statusupdated()), this, SLOT(on_statusupdated())) ;
    printer.updateStatus() ;

    // Timer ticks every second
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
    timer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::tick()
{
    if (!printer.isBusy()) printer.updateStatus() ;
}

void MainWindow::on_statusupdated()
{
    ui->textEditLog->clear() ;
    ui->textEditLog->append(printer.rawData()) ;

    if (!printer.isBusy() && printer.isPrinting()) {

        ui->progressBar->show() ;

        // TODO: Print in hours and minutes
        ui->labelStatus->setText(printer.status() + QString(" - elapsed: ") +
                                 QString::number(printer.elapsedMinutes()) + QString("min, remaining: ") +
                                 QString::number(printer.remainingMinutes()) + QString("min")) ; ;

        ui->progressBar->setValue(printer.percentComplete());

    } else {

        ui->progressBar->hide() ;
        ui->labelStatus->setText(printer.status()) ;
    }

    if (!printer.isBusy() && !printer.isError()) {

        ui->labelBedTemperature->setText(QString::number((double)printer.bedTemperature()) + QString("C")) ;
        ui->labelFilament1Remaining->setText(QString::number((double)printer.filamentRemaining(0), 'f', 1) + QString("m")) ;
        ui->labelTemperature1->setText(QString::number((double)printer.nozzleTemperature(0)) + QString("C")) ;

        if (printer.nozzles()>1) {
            ui->frameNozzle2->show() ;
            ui->labelFilament2Remaining->setText(QString::number((double)printer.filamentRemaining(1), 'f', 1) + QString("m")) ;
            ui->labelTemperature2->setText(QString::number((double)printer.nozzleTemperature(1)) + QString("C")) ;
        }

        ui->labelIPAdddress->setText(printer.ipAddress()) ;
        ui->labelModelSerial->setText(printer.modelNumber() + QString(" (") + printer.serialNumber() + QString(")")) ;

        if (printer.isDoorOpen() || printer.isLidOpen()) {
            ui->doorIcon->setPixmap(QPixmap(QString(":/Pictures/DoorOpen.png"))) ;
        } else {
            ui->doorIcon->setPixmap(QPixmap(QString(":/Pictures/Idle.png"))) ;
        }

    }

}

void MainWindow::on_action_Upload_triggered()
{

}

void MainWindow::on_action_Configure_triggered()
{
    configuration.show() ;
    if (configuration.exec()==1) { configuration.save() ; }
    else { configuration.load() ; }
    configuration.hide() ;
}

void MainWindow::on_action_Exit_triggered()
{
    this->close() ;
}


void MainWindow::on_CadPushButton_clicked()
{
    // TODO: Check for valid path to app
    // TODO: Check not already running
    cadProc.start(configuration.cadApp()) ;
}

void MainWindow::on_SlicePushButton_clicked()
{
    // TODO: Check for valid path to app
    // TODO: Check not already running
    sliceProc.start(configuration.sliceApp()) ;
}

void MainWindow::on_UploadPushButton_clicked()
{

}
