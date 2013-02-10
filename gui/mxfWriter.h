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

#ifndef __MXF_WRITER_H__
#define __MXF_WRITER_H__

#include <QtGui>
#include <QObject>
#include <string.h>
#include <iostream>
#include <opendcp.h>

class MxfWriter : public QThread
{
    Q_OBJECT

public:
    MxfWriter(QObject *parent);

    ~MxfWriter();
    void run();
    void init(opendcp_t *opendcp, QFileInfoList fileList, QString outputFile);
    int  rc;
    int  cancelled;

private:
    opendcp_t      *opendcpMxf;
    QFileInfoList   mxfFileList;
    QString         mxfOutputFile;
    void reset();
    static int frameDoneCb(void *data);

signals:
    void finished();
    void setResult(int);
    void errorMessage(QString);
    void frameDone();

public slots:
    void cancel();

private slots:

};

#endif // __MXF_WRITER_H__
