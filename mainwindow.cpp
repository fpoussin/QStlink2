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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdlib.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->isroot = false;
    this->ui->b_disconnect->setEnabled(false);
    this->ui->b_send->setEnabled(false);
    this->ui->b_receive->setEnabled(false);

    this->username = new QString(getenv("USER"));
    if (this->username != QString("root")) {
        this->ui->t_log->appendPlainText("You need to run the program as root in order to access USB subsystems. Use sudo.");
    }
    else {
        this->ui->t_log->appendPlainText("Running as root, good.");
        this->isroot = true;
    }

    this->stlink = new stlinkv2();
    this->devices = new DeviceList(this);
    stlink->moveToThread(&this->usbThread);
    this->stlink->start();
    qDebug() << "Thread running:" << this->stlink->isRunning() ;


    if (this->devices->IsLoaded() && this->isroot) {
        this->ui->gb_top->setEnabled(true);
        this->ui->t_log->appendPlainText(QString::number(this->devices->getDevicesCount())+" Device descriptions loaded.");
        QObject::connect(this->ui->b_connect, SIGNAL(clicked()), this, SLOT(Connect()));
        QObject::connect(this->ui->b_disconnect, SIGNAL(clicked()), this, SLOT(Disconnect()));
        QObject::connect(this->ui->b_send, SIGNAL(clicked()), this, SLOT(Send()));
        QObject::connect(this->ui->b_receive, SIGNAL(clicked()), this, SLOT(Receive()));
        QObject::connect(this->ui->b_halt, SIGNAL(clicked()), this, SLOT(HaltMCU()));
        QObject::connect(this->ui->b_run, SIGNAL(clicked()), this, SLOT(RunMCU()));
        QObject::connect(this->ui->b_reset, SIGNAL(clicked()), this, SLOT(ResetMCU()));
        QObject::connect(this->ui->r_jtag, SIGNAL(clicked()), this, SLOT(setModeJTAG()));
        QObject::connect(this->ui->r_swd, SIGNAL(clicked()), this, SLOT(setModeSWD()));
        QObject::connect(this->ui->b_getMCU, SIGNAL(clicked()), this, SLOT(getMCU()));
    }

    else {
        this->ui->t_log->appendPlainText("Could not load the devices list");
    }

}

MainWindow::~MainWindow()
{
    this->stlink->exit();
    delete stlink;
    delete devices;
    delete ui;
}


void MainWindow::Connect()
{
    this->ui->t_log->appendPlainText("Searching Device...");

    switch (this->stlink->connect()) {
    case -1:
        this->ui->t_log->appendPlainText("ST Link V2 not found.");
        return;
    default:
        this->ui->t_log->appendPlainText("ST Link V2 found!");
        this->stlink->setup();
        this->getVersion();
        this->stlink->setExitModeDFU();
        this->setModeSWD();
        this->getStatus();
        this->getMCU();
        this->ui->b_connect->setEnabled(false);
        this->ui->b_disconnect->setEnabled(true);
        this->ui->gb_bottom->setEnabled(true);
        this->ui->b_send->setEnabled(true);
        this->ui->b_receive->setEnabled(true);
        break;
    }

}

void MainWindow::Disconnect()
{
    this->ui->t_log->appendPlainText("Disconnecting...");
    this->stlink->disconnect();
    this->ui->t_log->appendPlainText("Disconnected.");
    this->ui->b_disconnect->setEnabled(false);
    this->ui->b_connect->setEnabled(true);
    this->ui->gb_bottom->setEnabled(false);
    this->ui->b_send->setEnabled(false);
    this->ui->b_receive->setEnabled(false);
}

void MainWindow::Send()
{
    this->filename.clear();
    this->filename = QFileDialog::getOpenFileName(this, "Open file", "/", "Binary Files (*.bin)");
    if (!this->filename.isNull()) {
        this->ui->t_log->appendPlainText("Sending from "+this->filename);
        QFile file(this->filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Could not open the file.");
            return;
        }
        this->ui->t_log->appendPlainText("Size: "+QString::number(file.size()/1024)+"KB");

        if (file.size() > this->stlink->flash_size) {
            this->dialog.setText("Warning", "The file is biggen then the flash size, continue?");
            if(dialog.exec() != QDialog::Accepted){
                return;
            }
        }

        else {
            this->dialog.setText("Confirm", "The flash memory will be erased and the new file programmed, continue ?");
            if(dialog.exec() != QDialog::Accepted){
                return;
            }
        }

        this->stlink->resetMCU(); // We stop the MCU
        this->ui->tabw_info->setCurrentIndex(3);
        this->ui->pgb_tranfer->setValue(0);
        this->ui->l_progress->setText("Starting transfer...");
        quint32 program_size = 4; // WORD (4 bytes)
        quint32 from = this->stlink->device->flash_base;
        quint32 to = this->stlink->device->flash_base+file.size();
        qDebug() << "Writing from" << QString::number(from, 16) << "to" << QString::number(to, 16);
        QByteArray buf;
        quint32 addr, progress;
        char buf2[program_size];

        // Unlock flash
        if (!this->stlink->unlockFlash())
            return;

        this->stlink->setProgramSize(4); // WORD (4 bytes)

        // Erase flash
        if (!this->stlink->setMassErase(true))
            return;

        this->ui->l_progress->setText("Erasing flash... This might take some time.");
        this->stlink->setSTRT();
        while(this->stlink->isBusy()) {
            usleep(500000); // 500ms
            QApplication::processEvents();
        }

        qDebug() << "\n";
        // Remove erase flash flag
        if (this->stlink->setMassErase(false))
            return;

        qDebug() << "\n";
        // We finally enable flash programming
        if (!this->stlink->setFlashProgramming(true))
            return;

        while(this->stlink->isBusy()) {
            usleep(100000); // 100ms
            QApplication::processEvents();
        }

        // Unlock flash again. Seems like this is mandatory.
        if (!this->stlink->unlockFlash())
            return;

        for (int i=0; i+program_size<=file.size(); i+=program_size)
        {

            QApplication::processEvents(); // Dirty hack
            buf.clear();

            if (file.atEnd()) { // This should never happen
                qDebug() << "EOF!";
                return;
            }

            file.read(buf2, program_size);
            buf.append(buf2, program_size);

            addr = this->stlink->device->flash_base+i;
            this->stlink->writeMem32(addr, buf);
            progress = (i*100)/file.size();
            this->ui->pgb_tranfer->setValue(progress);
            qDebug() << "Progress:"<< QString::number(progress)+"%";
            this->ui->l_progress->setText("Transfered "+QString::number(i/1024)+" kilobytes out of "+QString::number(file.size()/1024));
        }


        if (!file.atEnd()) {
            // last bytes of the file
            file.read(buf2, file.size() % program_size);
            buf.append(buf2, sizeof(buf2));
            addr = this->stlink->device->flash_base-file.size()-program_size;
            this->stlink->writeMem32(addr, buf);
        }
        this->ui->pgb_tranfer->setValue(100);
        this->ui->l_progress->setText("Transfer done");

        // We disable flash programming
        if (this->stlink->setFlashProgramming(false))
            return;

        // Lock flash
        this->stlink->lockFlash();

        this->stlink->resetMCU();
        this->stlink->runMCU();
        file.close();
    }

}

void MainWindow::Receive()
{
    this->filename.clear();
    this->filename = QFileDialog::getSaveFileName(this, "Save File", "/", "Binary Files (*.bin)");
    if (!this->filename.isNull()) {
        this->ui->t_log->appendPlainText("Saving to "+this->filename);
        QFile file(this->filename);
        if (!file.open(QIODevice::ReadWrite)) {
            qCritical("Could not save the file.");
            return;
        }
        this->stlink->resetMCU(); // We stop the MCU
        this->ui->tabw_info->setCurrentIndex(3);
        this->ui->pgb_tranfer->setValue(0);
        this->ui->l_progress->setText("Starting transfer...");
        quint32 program_size = this->stlink->device->flash_pgsize*4;
        quint32 from = this->stlink->device->flash_base;
        quint32 to = this->stlink->device->flash_base+from;
        qDebug() << "Reading from" << QString::number(from, 16) << "to" << QString::number(to, 16);
        quint32 addr, progress;

        for (int i=0; i<this->stlink->device->flash_size; i+=program_size)
        {
            QApplication::processEvents();// Dirty hack
            addr = this->stlink->device->flash_base+i;
            if (this->stlink->readMem32(addr, program_size) < 0)
                break;
            file.write(this->stlink->recv_buf);
            progress = (i*100)/this->stlink->device->flash_size;
            this->ui->pgb_tranfer->setValue(progress);
            qDebug() << "Progress:"<< QString::number(progress)+"%";
            this->ui->l_progress->setText("Transfered "+QString::number(i/1024)+" kilobytes out of "+QString::number(this->stlink->device->flash_size/1024));
        }
        this->ui->pgb_tranfer->setValue(100);
        this->ui->l_progress->setText("Transfer done");
        this->stlink->runMCU();
        file.close();
    }
}

void MainWindow::HaltMCU()
{
    this->ui->t_log->appendPlainText("Halting MCU...");
    this->stlink->haltMCU();
    this->getStatus();
}

void MainWindow::RunMCU()
{
    this->ui->t_log->appendPlainText("Resuming MCU...");
    this->stlink->runMCU();
    this->getStatus();
}

void MainWindow::ResetMCU()
{
    this->ui->t_log->appendPlainText("Reseting MCU...");
    this->stlink->resetMCU();
    this->getStatus();
}

void MainWindow::setModeJTAG()
{
    this->ui->t_log->appendPlainText("Changing mode to JTAG...");
    this->stlink->setModeJTAG();
    this->getMode();
}

void MainWindow::setModeSWD()
{
    this->ui->t_log->appendPlainText("Changing mode to SWD...");
    this->stlink->setModeSWD();
    this->getMode();
}

void MainWindow::getVersion()
{
    this->ui->t_log->appendPlainText("Fetching version...");
    this->stlink->getVersion();
}

void MainWindow::getMode()
{
    this->ui->t_log->appendPlainText("Fetching mode...");
    this->ui->t_log->appendPlainText(this->stlink->getMode());
}

void MainWindow::getStatus()
{
    this->ui->t_log->appendPlainText("Fetching status...");
    this->ui->t_log->appendPlainText(this->stlink->getStatus());
}

void MainWindow::getMCU()
{
    this->ui->t_log->appendPlainText("Fetching MCU Info...");
    this->stlink->getCoreID();
    this->stlink->getChipID();

    if (this->devices->search(this->stlink->chip_id)) {
        qDebug() << "Device type: " << this->devices->cur_device->type;
        this->stlink->device = this->devices->cur_device;

        this->ui->le_type->setText(this->stlink->device->type);
        this->ui->le_chipid->setText("0x"+QString::number(this->stlink->device->chip_id, 16));
        this->ui->le_flashbase->setText("0x"+QString::number(this->stlink->device->flash_base, 16));
        this->ui->le_flashsize->setText(QString::number(this->stlink->device->flash_size/1024)+"KB");
        this->ui->le_ramsize->setText(QString::number(this->stlink->device->sram_size/1024)+"KB");
        this->ui->le_rambase->setText("0x"+QString::number(this->stlink->device->sram_base, 16));

        this->ui->le_stlver->setText(QString::number(this->stlink->STLink_ver));
        this->ui->le_jtagver->setText(QString::number(this->stlink->JTAG_ver));
        this->ui->le_swimver->setText(QString::number(this->stlink->SWIM_ver));

        if(!this->stlink->JTAG_ver)
            this->ui->le_jtagver->setToolTip("Not supported");
        if(!this->stlink->SWIM_ver)
            this->ui->le_swimver->setToolTip("Not supported");
    }
    else this->ui->t_log->appendPlainText("Device not found!");
}
