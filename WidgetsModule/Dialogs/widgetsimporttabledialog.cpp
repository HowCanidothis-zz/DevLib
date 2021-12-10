#include "widgetsimporttabledialog.h"
#include "ui_widgetsimporttabledialog.h"

#include "WidgetsModule/Attachments/tableviewwidgetattachment.h"

WidgetsImportTableDialog::WidgetsImportTableDialog(QWidget *parent)
    : Super(parent)
    , ui(new Ui::WidgetsImportTableDialog)
{
    ui->setupUi(this);
    ui->ImportView->OnTransitionStarted.Connect(this, [this]{
        ui->BtnInsert->setEnabled(false);
        ui->BtnReplace->setEnabled(false);
    });
    ui->ImportView->OnTransited.Connect(this, [this]{
        ui->BtnInsert->setEnabled(true);
        ui->BtnReplace->setEnabled(true);
    });
}

WidgetsImportTableDialog::~WidgetsImportTableDialog()
{
    delete ui;
}

WidgetsImportView* WidgetsImportTableDialog::GetView() const
{
    return ui->ImportView;
}

void WidgetsImportTableDialog::on_BtnReplace_clicked()
{
    done(IR_Replaced);
}

void WidgetsImportTableDialog::on_BtnInsert_clicked()
{
    done(IR_Inserted);
}

void WidgetsImportTableDialog::on_BtnCancel_clicked()
{
    done(IR_Canceled);
}
