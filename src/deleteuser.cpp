#include "deleteuser.hpp"
#include "ui_deleteuser.h"

#include "databasemanager.hpp"
#include <QTimer>

DeleteUser::DeleteUser(DatabaseManager* dbmanager, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeleteUser)
{
    ui->setupUi(this);
    this->setAttribute( Qt::WA_DeleteOnClose, true );
    m_dbmanager = dbmanager;
}

DeleteUser::~DeleteUser()
{
    delete ui;
}

void DeleteUser::on_pushButton_delete_clicked()
{
    QString errormsg;
    if(validateForm()){
        if(!m_dbmanager->DeleteUser(ui->lineEdit_deleteUsername->text(),errormsg)) ui->label_error->setText(errormsg);
        else{
            this->setEnabled(false);
            ui->label_error->setStyleSheet("color:green;");
            ui->label_error->setText("User is deleted.");
            QTimer::singleShot(2000,this,&QDialog::close);
        }
    }
}

void DeleteUser::on_pushButton_cancel_clicked()
{
    this->close();
}

bool DeleteUser::validateForm()
{
    if(ui->lineEdit_deleteUsername->text().isEmpty()) return false;
    return true;
}
