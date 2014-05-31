/*
    This file is part of QSTLink2.

    QSTLink2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QSTLink2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QSTLink2.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    mUi(new Ui::Dialog)
{
    mUi->setupUi(this);
//    this->ui->l_text->setStyleSheet(QString::fromUtf8("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 0, 0, 0), stop:1 rgba(255, 255, 255, 0));"));
    mUi->l_version->setText(__QSTL_VER__);
    mUi->l_cmpdate->setText(QString(__DATE__)+" "+QString(__TIME__));
}

Dialog::~Dialog()
{
    delete mUi;
}

void Dialog::setText(const QString &title, const QString &text)
{
    this->setWindowTitle(title);
    mUi->l_text->setText(text);
}

void Dialog::setHTML(const QString &title, const QString &html)
{
    this->setWindowTitle(title);
    mUi->l_text->setHtml(html);
}
