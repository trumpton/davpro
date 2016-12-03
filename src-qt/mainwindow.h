#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include "printerinterface.h"
#include "configuration.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    PrinterInterface printer ;
    Configuration configuration ;
    QProcess cadProc, sliceProc ;

private slots:
    void tick() ;

    void on_statusupdated() ;
    void on_action_Upload_triggered();
    void on_action_Configure_triggered();
    void on_action_Exit_triggered();
    void on_CadPushButton_clicked();
    void on_SlicePushButton_clicked();
    void on_UploadPushButton_clicked();
};

#endif // MAINWINDOW_H
