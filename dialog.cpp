#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::setText(const QString &title, const QString &text)
{
    this->ui->l_title->setText(title);
    this->ui->l_text->setText(text);
}
