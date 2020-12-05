#include "manualvehicleentry.hpp"
#include "ui_manualvehicleentry.h"

#include "databasemanager.hpp"
#include "applicationwindow.hpp"
#include <QTimer>

ManualVehicleEntry::ManualVehicleEntry(DatabaseManager* dbmanager, const qint32& currentPlanID, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ManualVehicleEntry)
{
    ui->setupUi(this);
    this->setAttribute( Qt::WA_DeleteOnClose, true );
    m_dbmanager = dbmanager;
    m_currentPlanID = currentPlanID;
    QStringList colors;
    for(qint32 id : m_dbmanager->getColors()) colors.append(m_dbmanager->getColors().key(id));
    ui->comboBox_colors->addItems(colors);
    QStringList types;
    for(qint32 id : m_dbmanager->getVehicleTypes()) types.append(m_dbmanager->getVehicleTypes().key(id));
    ui->comboBox_types->addItems(types);

}

ManualVehicleEntry::~ManualVehicleEntry()
{
    delete ui;
}

void ManualVehicleEntry::on_pushButton_clicked()
{
    if(validateForm()){
        QString errormsg;
        qint32 vehicleid;
        QString plate = ui->lineEdit_plate->text();
        plate = plate.simplified();
        plate = plate.replace(" ","");
        plate = plate.toUpper();
        if(!m_dbmanager->NewVehicleEntry(plate,errormsg,vehicleid,ui->lineEdit_model->text(),ui->comboBox_types->currentText(),ui->comboBox_colors->currentText())){
            ui->label_error->setText(errormsg);
            return;
        }
        if(!m_dbmanager->NewPaymentEntry(vehicleid,m_currentPlanID,errormsg)){
            ui->label_error->setText(errormsg);
            return;
        }
        ui->pushButton->setEnabled(false);
        ui->label_error->setStyleSheet("color:green;");
        ui->label_error->setText("New entry is saved.");
        emit decreaseCount();
        QTimer::singleShot(2000,this,&QDialog::close);
    }
}

bool ManualVehicleEntry::validateForm()
{
    if(ui->lineEdit_plate->text().isEmpty()){
        ui->label_error->setText("ENTER PLATE");
        return false;
    }
    return true;
}
