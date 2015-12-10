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
#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QString>
#include <QColor>
#include <QStyle>

namespace Ui {
    class Dialog;
}

/**
 * @brief
 *
 */
class Dialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief
     *
     * @param parent
     */
    explicit Dialog(QWidget *parent = 0);
    /**
     * @brief
     *
     */
    ~Dialog();
    /**
     * @brief
     *
     * @param title
     * @param text
     */
    void setText(const QString &title, const QString &text);
    /**
     * @brief
     *
     * @param title
     * @param html
     */
    void setHTML(const QString &title, const QString &html);
    Ui::Dialog *mUi; /**< TODO: describe */
};

#endif // DIALOG_H
