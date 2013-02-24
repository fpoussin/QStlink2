#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
//    this->ui->l_text->setStyleSheet(QString::fromUtf8("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 0, 0, 0), stop:1 rgba(255, 255, 255, 0));"));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::setText(const QString &title, const QString &text)
{
    this->setWindowTitle(title);
    this->ui->l_text->setText(text);
}

void Dialog::setHTML(const QString &title, const QString &html)
{
    this->setWindowTitle(title);
    this->ui->l_text->setHtml(html);
}
