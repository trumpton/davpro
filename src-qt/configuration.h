#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QDialog>
#include <QSettings>


namespace Ui {
class Configuration;
}

class Configuration : public QDialog
{
    Q_OBJECT

public:
    explicit Configuration(QWidget *parent = 0);
    ~Configuration();
    void load() ;
    void save() ;

    QString ipAddress() ;
    QString working() ;
    QString cadApp() ;
    QString sliceApp() ;

private:
    Ui::Configuration *ui;
    QSettings *settings ;
};

#endif // CONFIGURATION_H
