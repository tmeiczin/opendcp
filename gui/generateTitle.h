/*
     OpenDCP: Builds Digital Cinema Packages
     Copyright (c) 2010-2011 Terrence Meiczinger, All Rights Reserved

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GENERATETITLE_H
#define GENERATETITLE_H

#include <QDialog>

namespace Ui {
    class GenerateTitle;
}

class GenerateTitle : public QDialog
{
    Q_OBJECT

public slots:
    void updateTitle();

protected:
    void load(const QString& filename);

public:
    explicit GenerateTitle(QWidget *parent = 0);
    ~GenerateTitle();
    QString getTitle();

private:
    Ui::GenerateTitle *ui;
};

#endif // GENERATETITLE_H
