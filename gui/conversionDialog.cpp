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

#include <QDir>
#include "conversionDialog.h"

enum CONVERSION_STATE {
    IDLE  = 0,
    START,
    STOP,
    RUN
};

ConversionDialog::ConversionDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::WindowMinimizeButtonHint);

    connect(buttonClose, SIGNAL(clicked()), this, SLOT(close()));
    connect(buttonStop,  SIGNAL(clicked()), this, SLOT(stop()));
}

void ConversionDialog::init(int nFrames, int threadCount)
{
    QString labelText;

    m_currentCount = 0;
    m_canceled     = false;
    m_totalCount   = nFrames;
    m_result       = 0;

    progressBar->reset();
    progressBar->setRange(0, nFrames);

    labelText.sprintf("Conversion using %d threads(s)", threadCount);
    labelThreadCount->setText(labelText);

    setButtons(RUN);
}

void ConversionDialog::stop()
{
    setButtons(STOP);
    labelTotal->setText("Cancelling...");
    m_canceled = true;
    emit cancel();
}

void ConversionDialog::update()
{
    QString labelText;

    if (m_canceled == true) {
        return;
    }

    labelText.sprintf("Completed frame %d of %d.",m_currentCount, m_totalCount);

    labelTotal->setText(labelText);
    progressBar->setValue(m_currentCount);

    // make sure current doesn't exceed total (shouldn't happen)
    if (m_currentCount < m_totalCount) {
        m_currentCount++;
    }
}

void ConversionDialog::setResult(int result) {
    m_result = result;
}

void ConversionDialog::finished()
{
    QString t;
    QString labelText;

    if (m_canceled == true) {
        t = "canceled.";
    } else if (m_result){
        t = "failed.";
    } else {
        t = "completed.";
        m_currentCount = m_totalCount;
    }

    progressBar->setValue(m_currentCount);
    labelText.sprintf("Completed %d of %d. Conversion ",m_currentCount, m_totalCount);
    labelText.append(t);
    labelTotal->setText(labelText);
    setButtons(IDLE);
}

void ConversionDialog::setButtons(int state)
{
    switch (state)
    {
        case IDLE:
            buttonClose->setEnabled(true);
            buttonStop->setEnabled(false);
            break;
        case RUN:
            buttonClose->setEnabled(false);
            buttonStop->setEnabled(true);
            break;
        case STOP:
            buttonClose->setEnabled(false);
            buttonStop->setEnabled(false);
            break;
    }
}
