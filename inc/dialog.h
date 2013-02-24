#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QString>
#include <QColor>
#include <QStyle>

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    void setText(const QString &title, const QString &text);
    void setHTML(const QString &title, const QString &html);
    Ui::Dialog *ui;
};

#endif // DIALOG_H
