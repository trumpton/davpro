
#include "configuration.h"
#include "ui_configuration.h"

#define COMPANY "trumpton"
#define APPLICATION "dvpro"

Configuration::Configuration(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Configuration)
{
    ui->setupUi(this);
    load() ;
}

Configuration::~Configuration()
{
    delete ui;
}

void Configuration::load()
{
    settings = new QSettings(COMPANY, APPLICATION) ;
    ui->lineEdit_IPAddress->setText(ipAddress()) ;
    ui->lineEdit_DefaultFolder->setText(working()) ;
    ui->lineEdit_CadTool->setText(cadApp()) ;
    ui->lineEdit_SlicerTool->setText(sliceApp()) ;
}

void Configuration::save()
{
    settings->setValue("ipaddress", ui->lineEdit_IPAddress->text()) ;
    settings->setValue("working", ui->lineEdit_DefaultFolder->text()) ;
    settings->setValue("cadtool", ui->lineEdit_CadTool->text()) ;
    settings->setValue("slicertool", ui->lineEdit_SlicerTool->text()) ;

}

QString Configuration::ipAddress()
{
    return settings->value("ipaddress").toString() ;
}

QString Configuration::working()
{
    return settings->value("working").toString() ;
}

QString Configuration::cadApp()
{
    return settings->value("cadtool").toString() ;
}

QString Configuration::sliceApp()
{
    return settings->value("slicertool").toString() ;
}
