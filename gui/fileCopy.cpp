/*
     : Builds Digital Cinema Packages
     Copyright (c) 2010-2012 Terrence Meiczinger, All Rights Reserved

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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui>
#include <QDir>
#include <stdio.h>
#include <stdlib.h>

void MainWindow::fileCopy(QString source, QString destination)
{
    int pg = 0;

    QFile src(source);
    QFile dst(destination);

    if (!src.open(QFile::ReadOnly) || !dst.open(QFile::WriteOnly)) {
        return;
    }

    qint64 len = src.bytesAvailable();

    QProgressDialog *progress = new QProgressDialog("Copy in progress.", "Cancel", 0, len, this);
    progress->setWindowModality(Qt::WindowModal);

    while (!src.atEnd()) {
        QByteArray bytearray = src.read(4096);
        dst.write(bytearray);
        qint64 act = bytearray.length();
        pg = pg + act;
        progress->setValue(pg);
        if (progress->wasCanceled()) {
            progress->hide();
            break;
        }
        qApp->processEvents();
    }
}
