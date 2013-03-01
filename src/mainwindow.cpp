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
#include <mainwindow.h>
#include <ui_mainwindow.h>
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
    this->ui->b_verify->setEnabled(false);
    this->stlink = new stlinkv2();
    this->devices = new DeviceList(this);
    this->tfThread = new transferThread();

    if (this->devices->IsLoaded()) {

        this->ui->gb_top->setEnabled(true);
        this->log(QString::number(this->devices->getDevicesCount())+" Device descriptions loaded.");
        QObject::connect(this->ui->b_quit,SIGNAL(clicked()),qApp,SLOT(quit()));
        QObject::connect(this->ui->b_qt,SIGNAL(clicked()),qApp,SLOT(aboutQt()));
        QObject::connect(this->ui->b_connect, SIGNAL(clicked()), this, SLOT(Connect()));
        QObject::connect(this->ui->b_disconnect, SIGNAL(clicked()), this, SLOT(Disconnect()));
        QObject::connect(this->ui->b_send, SIGNAL(clicked()), this, SLOT(Send()));
        QObject::connect(this->ui->b_receive, SIGNAL(clicked()), this, SLOT(Receive()));
        QObject::connect(this->ui->b_verify, SIGNAL(clicked()), this, SLOT(Verify()));
        QObject::connect(this->ui->b_halt, SIGNAL(clicked()), this, SLOT(HaltMCU()));
        QObject::connect(this->ui->b_run, SIGNAL(clicked()), this, SLOT(RunMCU()));
        QObject::connect(this->ui->b_reset, SIGNAL(clicked()), this, SLOT(ResetMCU()));
        QObject::connect(this->ui->r_jtag, SIGNAL(clicked()), this, SLOT(setModeJTAG()));
        QObject::connect(this->ui->r_swd, SIGNAL(clicked()), this, SLOT(setModeSWD()));
        QObject::connect(this->ui->b_hardReset, SIGNAL(clicked()), this, SLOT(HardReset()));

        // Thread
        QObject::connect(this->tfThread, SIGNAL(sendProgress(quint32)), this, SLOT(updateProgress(quint32)));
        QObject::connect(this->tfThread, SIGNAL(sendStatus(QString)), this, SLOT(updateStatus(QString)));
        QObject::connect(this->tfThread, SIGNAL(sendLock(bool)), this, SLOT(lockUI(bool)));
        QObject::connect(this->ui->b_stop, SIGNAL(clicked()), this->tfThread, SLOT(halt()));
        QObject::connect(this->tfThread, SIGNAL(sendLog(QString)), this, SLOT(log(QString)));

        // Help
        QObject::connect(this->ui->b_help, SIGNAL(clicked()), this, SLOT(showHelp()));
    }

    else {
        this->log("Could not load the devices list");
    }
}

MainWindow::~MainWindow()
{
    this->tfThread->exit();
    delete tfThread;
    delete stlink;
    delete devices;
    delete ui;
}

void MainWindow::showHelp()
{

    this->dialog.setText("Help","Could no load help file");

    QFile file(":/help.html");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the help file.");
    }

    this->dialog.setHTML(QString("Help"), QString(file.readAll()));
    this->dialog.show();
}

bool MainWindow::Connect()
{
    this->log("Searching Device...");

    switch (this->stlink->connect()) {
    case -1:
        this->log("ST Link V2 not found or unable to access it.\nCheck the logs.");
        return false;
    default:
        this->log("ST Link V2 found!");
        this->getVersion();
        this->stlink->setExitModeDFU();
        if (this->ui->r_jtag->isChecked())
            this->setModeJTAG();
        else
            this->setModeSWD();
        this->getStatus();
        this->ui->b_connect->setEnabled(false);
        this->ui->b_disconnect->setEnabled(true);
        if (this->getMCU()) {
            this->ui->gb_bottom->setEnabled(true);
            this->ui->b_send->setEnabled(true);
            this->ui->b_receive->setEnabled(true);
            this->ui->b_verify->setEnabled(true);
            return true;
        }
        else
            return false;
    }
    return false;
}

void MainWindow::Disconnect()
{
    this->log("Disconnecting...");
    this->stlink->disconnect();
    this->log("Disconnected.");
    qInformal() << "Disconnected.";
    this->ui->b_disconnect->setEnabled(false);
    this->ui->b_connect->setEnabled(true);
    this->ui->gb_bottom->setEnabled(false);
    this->ui->b_send->setEnabled(false);
    this->ui->b_receive->setEnabled(false);
    this->ui->b_verify->setEnabled(false);
}

void MainWindow::log(const QString &s)
{
    this->ui->t_log->appendPlainText(s);
}

void MainWindow::lockUI(bool enabled)
{
    this->ui->gb_top->setEnabled(!enabled);
    this->ui->gb_bottom->setEnabled(!enabled);
}

void MainWindow::updateProgress(const quint32 &p)
{
    this->ui->pgb_transfer->setValue(p);
}

void MainWindow::updateStatus(const QString &s)
{
    this->ui->l_progress->setText(s);
}

void MainWindow::Send()
{
    this->filename.clear();
    this->filename = QFileDialog::getOpenFileName(this, "Open file", "", "Binary Files (*.bin)");
    if (!this->filename.isNull()) {
        this->log("Sending from "+this->filename);
        QFile file(this->filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Could not open the file.");
            return;
        }
        this->log("Size: "+QString::number(file.size()/1024)+"KB");

        if (file.size() > (*this->stlink->device)["flash_size"]) {
            this->dialog.setText("Warning", "The file is bigger than the flash size!\nThe flash memory will be erased and the new file programmed, continue?");
            if(dialog.exec() != QDialog::Accepted){
                return;
            }
        }
        else {
            this->dialog.setText("Confirm", "The flash memory will be erased and the new file programmed, continue?");
            if(dialog.exec() != QDialog::Accepted){
                return;
            }
        }
        file.close();

        this->Send(this->filename, true);
    }
}

void MainWindow::Send(const QString &path, const bool &erase)
{
    qDebug("Writing flash");
    this->stlink->resetMCU(); // We stop the MCU
    this->ui->tabw_info->setCurrentIndex(3);
    this->ui->pgb_transfer->setValue(0);
    this->ui->l_progress->setText("Starting transfer...");

    // Transfer thread
    this->tfThread->setParams(this->stlink, path, true, erase, false);
    this->tfThread->start();
}

void MainWindow::Receive()
{
    qDebug("Reading flash");
    this->filename.clear();
    this->filename = QFileDialog::getSaveFileName(this, "Save File", "", "Binary Files (*.bin)");
    if (!this->filename.isNull()) {
        this->log("Saving to "+this->filename);
        QFile file(this->filename);
        if (!file.open(QIODevice::ReadWrite)) {
            qCritical("Could not save the file.");
            return;
        }
        file.close();
        this->Receive(this->filename);
    }
}

void MainWindow::Receive(const QString &path)
{
    this->ui->tabw_info->setCurrentIndex(3);
    this->ui->pgb_transfer->setValue(0);
    this->ui->l_progress->setText("Starting transfer...");

    // Transfer thread
    this->tfThread->setParams(this->stlink, path, false, false, false);
    this->tfThread->start();
}

void MainWindow::Verify()
{
    qDebug("Reading flash");
    this->filename.clear();
    this->filename = QFileDialog::getOpenFileName(this, "Open file", "", "Binary Files (*.bin)");
    if (!this->filename.isNull()) {
        this->log("Verifying "+this->filename);
        QFile file(this->filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Could not open the file.");
            return;
        }
        file.close();
        this->Verify(this->filename);
    }
}

void MainWindow::Verify(const QString &path)
{
    this->ui->tabw_info->setCurrentIndex(3);
    this->ui->pgb_transfer->setValue(0);
    this->ui->l_progress->setText("Starting Verification...");

    // Transfer thread
    this->tfThread->setParams(this->stlink, path, false, false, true);
    this->tfThread->start();
}

void MainWindow::eraseFlash()
{
    this->stlink->eraseFlash();
}

void MainWindow::HaltMCU()
{
    this->log("Halting MCU...");
    this->stlink->haltMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::RunMCU()
{
    this->log("Resuming MCU...");
    this->stlink->runMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::ResetMCU()
{
    this->log("Reseting MCU...");
    this->stlink->resetMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::HardReset()
{
    this->log("Hard Reset...");
    this->stlink->hardResetMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::setModeJTAG()
{
    if (!this->stlink->isConnected())
        return;
    this->log("Changing mode to JTAG...");
    this->stlink->setModeJTAG();
    usleep(100000);
    this->getMode();
}

void MainWindow::setModeSWD()
{
    if (!this->stlink->isConnected())
        return;
    this->log("Changing mode to SWD...");
    this->stlink->setModeSWD();
    usleep(100000);
    this->getMode();
}

void MainWindow::getVersion()
{
    this->log("Fetching version...");
    this->stlink->getVersion();
}

void MainWindow::getMode()
{
    this->log("Fetching mode...");
    this->log(this->stlink->getMode());
}

void MainWindow::getStatus()
{
    this->log("Fetching status...");
    this->log(this->stlink->getStatus());
}

bool MainWindow::getMCU()
{
    this->log("Fetching MCU Info...");
    this->stlink->getCoreID();
    this->stlink->resetMCU();
    this->stlink->getChipID();

    if (this->devices->search(this->stlink->chip_id)) {
        this->stlink->device = this->devices->cur_device;
        qInformal() << "Device type: " << this->stlink->device->type;

        this->ui->le_type->setText(this->stlink->device->type);
        this->ui->le_chipid->setText("0x"+QString::number((*this->stlink->device)["chip_id"], 16));
        this->ui->le_flashbase->setText("0x"+QString::number((*this->stlink->device)["flash_base"], 16));
        //this->ui->le_flashsize->setText(QString::number((*this->stlink->device)["flash_size"]/1024)+"KB");
        this->ui->le_ramsize->setText(QString::number((*this->stlink->device)["sram_size"]/1024)+"KB");
        this->ui->le_rambase->setText("0x"+QString::number((*this->stlink->device)["sram_base"], 16));

        this->ui->le_stlver->setText(QString::number(this->stlink->STLink_ver));
        this->ui->le_jtagver->setText(QString::number(this->stlink->JTAG_ver));
        this->ui->le_swimver->setText(QString::number(this->stlink->SWIM_ver));

        if(!this->stlink->JTAG_ver)
            this->ui->le_jtagver->setToolTip("Not supported");
        if(!this->stlink->SWIM_ver)
            this->ui->le_swimver->setToolTip("Not supported");

        (*this->stlink->device)["flash_size"] = this->stlink->readFlashSize();
        this->ui->le_flashsize->setText(QString::number((*this->stlink->device)["flash_size"])+"KB");

        return true;
    }
    this->log("Device not found in database!");
    qCritical() << "Device not found in database!";
    return false;
}
