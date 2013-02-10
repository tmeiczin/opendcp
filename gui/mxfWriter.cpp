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

#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>

#include <opendcp.h>
#include "mxfWriter.h"

MxfWriter::MxfWriter(QObject *parent)
    : QThread(parent)
{
    reset();
}

MxfWriter::~MxfWriter()
{

}

void MxfWriter::reset()
{
    cancelled = 0;
    rc        = 0;
}

void MxfWriter::cancel() {
    cancelled = 1;
}

int MxfWriter::frameDoneCb(void *data) {
    MxfWriter *self = static_cast<MxfWriter*>(data);
    emit self->frameDone();

    return self->cancelled;
}

//void MxfWriter::writeDoneCb(void *data) {
//    MxfWriter *self = static_cast<MxfWriter*>(data);
//    emit self->writeDone();
//}

void MxfWriter::init(opendcp_t *opendcp, QFileInfoList fileList, QString outputFile)
{
    opendcpMxf    = opendcp;
    mxfFileList   = fileList;
    mxfOutputFile = outputFile;
    reset();
}

void MxfWriter::run()
{
    int i = 0;

    opendcpMxf->mxf.frame_done.callback = MxfWriter::frameDoneCb;
    opendcpMxf->mxf.frame_done.argument = this;

    filelist_t *fileList = filelist_alloc(mxfFileList.size());

    while (!mxfFileList.isEmpty()) {
        sprintf(fileList->files[i++],"%s",mxfFileList.takeFirst().absoluteFilePath().toUtf8().data());
    }

    rc = write_mxf(opendcpMxf, fileList, mxfOutputFile.toUtf8().data());

    filelist_free(fileList);

    emit setResult(rc);
    emit finished();
}
