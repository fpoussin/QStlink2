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
    mUi(new Ui::MainWindow)
{
    mUi->setupUi(this);
    this->mUi->b_disconnect->setEnabled(false);
    this->mUi->gb_top->setEnabled(false);
    this->mUi->b_send->setEnabled(false);
    this->mUi->b_receive->setEnabled(false);
    this->mUi->b_verify->setEnabled(false);
    this->mUi->b_repeat->setEnabled(false);
    this->mStlink = new stlinkv2();
    this->mDevices = new DeviceList(this);
    this->tfThread = new transferThread();

    this->mLastAction = ACTION_NONE;

    if (this->mDevices->IsLoaded()) {

        this->mUi->gb_top->setEnabled(true);
        this->log(QString::number(this->mDevices->getDevicesCount())+" Device descriptions loaded.");
        QObject::connect(this->mUi->b_quit,SIGNAL(clicked()),this,SLOT(Quit()));
        QObject::connect(this->mUi->b_qt,SIGNAL(clicked()),qApp,SLOT(aboutQt()));
        QObject::connect(this->mUi->b_connect, SIGNAL(clicked()), this, SLOT(Connect()));
        QObject::connect(this->mUi->b_disconnect, SIGNAL(clicked()), this, SLOT(Disconnect()));
        QObject::connect(this->mUi->b_send, SIGNAL(clicked()), this, SLOT(Send()));
        QObject::connect(this->mUi->b_receive, SIGNAL(clicked()), this, SLOT(Receive()));
        QObject::connect(this->mUi->b_verify, SIGNAL(clicked()), this, SLOT(Verify()));
        QObject::connect(this->mUi->b_repeat, SIGNAL(clicked()), this, SLOT(Repeat()));
        QObject::connect(this->mUi->b_halt, SIGNAL(clicked()), this, SLOT(HaltMCU()));
        QObject::connect(this->mUi->b_run, SIGNAL(clicked()), this, SLOT(RunMCU()));
        QObject::connect(this->mUi->b_reset, SIGNAL(clicked()), this, SLOT(ResetMCU()));
        QObject::connect(this->mUi->r_jtag, SIGNAL(clicked()), this, SLOT(setModeJTAG()));
        QObject::connect(this->mUi->r_swd, SIGNAL(clicked()), this, SLOT(setModeSWD()));
        QObject::connect(this->mUi->b_hardReset, SIGNAL(clicked()), this, SLOT(HardReset()));

        // Thread
        QObject::connect(this->tfThread, SIGNAL(sendProgress(quint32)), this, SLOT(updateProgress(quint32)));
        QObject::connect(this->tfThread, SIGNAL(sendStatus(QString)), this, SLOT(updateStatus(QString)));
        QObject::connect(this->tfThread, SIGNAL(sendLoaderStatus(QString)), this, SLOT(updateLoaderStatus(QString)));
        QObject::connect(this->mStlink, SIGNAL(bufferPct(quint32)), this, SLOT(updateLoaderPct(quint32)));
        QObject::connect(this->tfThread, SIGNAL(sendLock(bool)), this, SLOT(lockUI(bool)));
        QObject::connect(this->mUi->b_stop, SIGNAL(clicked()), this->tfThread, SLOT(halt()));
        QObject::connect(this->tfThread, SIGNAL(sendLog(QString)), this, SLOT(log(QString)));

        // Help
        QObject::connect(this->mUi->b_help, SIGNAL(clicked()), this, SLOT(showHelp()));
    }

    else {
        this->log("Could not load the devices list");
    }
}

MainWindow::~MainWindow()
{
    this->tfThread->exit();
    delete tfThread;
    delete mStlink;
    delete mDevices;
    delete mUi;
}

void MainWindow::showHelp()
{

    this->mDialog.setText("Help","Could no load help file");

    QFile file(":/help.html");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the help file.");
    }

    this->mDialog.setHTML(QString("Help"), QString(file.readAll()));
    this->mDialog.show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    (void)event;
    this->Quit();
}

bool MainWindow::Connect()
{
    PrintFuncName();
    this->log("Searching Device...");

    qint32 ret = this->mStlink->connect();

    if (ret < 0) {
        this->log("ST Link V2 not found or unable to access it.");
#if defined(QWINUSB) && defined(WIN32)
        this->log("Did you install the official ST-Link V2 driver ?");
#elif !defined(WIN32)
        this->log("Did you install the udev rules ?");
#endif
        this->log("USB error: "+QString::number(ret));
        return false;

    }

    else {
        this->log("ST Link V2 found!");
        this->getVersion();
        this->mStlink->setExitModeDFU();
        if (this->mUi->r_jtag->isChecked())
            this->setModeJTAG();
        else
            this->setModeSWD();
        this->getStatus();
        this->mUi->b_connect->setEnabled(false);
        this->mUi->b_disconnect->setEnabled(true);
        if (this->getMCU()) {
            this->mUi->gb_bottom->setEnabled(true);
            this->mUi->b_send->setEnabled(true);
            this->mUi->b_receive->setEnabled(true);
            this->mUi->b_verify->setEnabled(true);
            this->mUi->b_repeat->setEnabled(true);
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
    this->mStlink->disconnect();
    this->log("Disconnected.");
    qInformal() << "Disconnected.";
    this->mUi->b_disconnect->setEnabled(false);
    this->mUi->b_connect->setEnabled(true);
    this->mUi->gb_bottom->setEnabled(false);
    this->mUi->b_send->setEnabled(false);
    this->mUi->b_receive->setEnabled(false);
    this->mUi->b_verify->setEnabled(false);
    this->mUi->b_repeat->setEnabled(false);
}

void MainWindow::log(const QString &s)
{
    this->mUi->t_log->appendPlainText(s);
}

void MainWindow::lockUI(bool enabled)
{
    this->mUi->gb_top->setEnabled(!enabled);
    this->mUi->gb_bottom->setEnabled(!enabled);
}

void MainWindow::updateProgress(quint32 p)
{
    this->mUi->pgb_transfer->setValue(p);
}

void MainWindow::updateStatus(const QString &s)
{
    this->mUi->l_progress->setText(s);
}

void MainWindow::updateLoaderStatus(const QString &s)
{
    this->mUi->l_status->setText(s);
}

void MainWindow::updateLoaderPct(quint32 p)
{
    this->mUi->pgb_loader->setValue(p);
}

void MainWindow::Send()
{
    this->mFilename.clear();
    this->mFilename = QFileDialog::getOpenFileName(this, "Open file", "", "Binary Files (*.bin)");
    if (!this->mFilename.isNull()) {
        QFile file(this->mFilename);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Could not open the file.");
            return;
        }
        this->log("Size: "+QString::number(file.size()/1024)+"KB");

        if (file.size() > (*this->mStlink->mDevice)["flash_size"]*1024) {
            if(QMessageBox::question(this, "Flash size exceeded", "The file is bigger than the flash size!\n\nThe flash memory will be erased and the new file programmed, continue?", QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes){
                return;
            }
        }
        else {
            if(QMessageBox::question(this, "Confirm", "The flash memory will be erased and the new file programmed, continue?", QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes){
                return;
            }
        }
        file.close();

        this->Send(this->mFilename);
        this->mLastAction = ACTION_SEND;
    }
}

void MainWindow::Send(const QString &path)
{
    qDebug("Writing flash");
    this->log("Sending "+path);
    this->mStlink->resetMCU(); // We stop the MCU
    this->mUi->tabw_info->setCurrentIndex(3);
    this->mUi->pgb_transfer->setValue(0);
    this->mUi->l_progress->setText("Starting transfer...");

    // Transfer thread
    this->tfThread->setParams(this->mStlink, path, true, false);
    this->tfThread->start();
}

void MainWindow::Receive()
{
    qDebug("Reading flash");
    this->mFilename.clear();
    this->mFilename = QFileDialog::getSaveFileName(this, "Save File", "", "Binary Files (*.bin)");
    if (!this->mFilename.isNull()) {
        QFile file(this->mFilename);
        if (!file.open(QIODevice::ReadWrite)) {
            qCritical("Could not save the file.");
            return;
        }
        file.close();
        this->Receive(this->mFilename);
        this->mLastAction = ACTION_RECEIVE;
    }
}

void MainWindow::Receive(const QString &path)
{
    this->log("Saving to "+path);
    this->mUi->tabw_info->setCurrentIndex(3);
    this->mUi->pgb_transfer->setValue(0);
    this->mUi->l_progress->setText("Starting transfer...");

    // Transfer thread
    this->tfThread->setParams(this->mStlink, path, false, false);
    this->tfThread->start();
}

void MainWindow::Verify()
{
    qDebug("Verify flash");
    this->mFilename.clear();
    this->mFilename = QFileDialog::getOpenFileName(this, "Open file", "", "Binary Files (*.bin)");
    if (!this->mFilename.isNull()) {
        QFile file(this->mFilename);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Could not open the file.");
            return;
        }
        file.close();
        this->Verify(this->mFilename);
        this->mLastAction = ACTION_VERIFY;
    }
}

void MainWindow::Repeat()
{
    switch (this->mLastAction) {

        case ACTION_SEND:
            this->Send(this->mFilename);
            break;
        case ACTION_RECEIVE:
            this->Receive(this->mFilename);
            break;
        case ACTION_VERIFY:
            this->Verify(this->mFilename);
            break;
        case ACTION_NONE:
            this->log("Nothing to repeat.");
            break;
        default:
            break;
    }
}

void MainWindow::Verify(const QString &path)
{
    this->log("Verifying "+path);
    this->mUi->tabw_info->setCurrentIndex(3);
    this->mUi->pgb_transfer->setValue(0);
    this->mUi->l_progress->setText("Starting Verification...");

    // Transfer thread
    this->tfThread->setParams(this->mStlink, path, false, true);
    this->tfThread->start();
}

void MainWindow::eraseFlash()
{
    this->mStlink->hardResetMCU();
    this->mStlink->resetMCU();
    if (!this->mStlink->unlockFlash())
        return;
    this->mStlink->eraseFlash();
}

void MainWindow::HaltMCU()
{
    this->log("Halting MCU...");
    this->mStlink->haltMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::RunMCU()
{
    this->log("Resuming MCU...");
    this->mStlink->runMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::ResetMCU()
{
    this->log("Reseting MCU...");
    this->mStlink->resetMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::HardReset()
{
    this->log("Hard Reset...");
    this->mStlink->hardResetMCU();
    usleep(100000);
    this->getStatus();
}

void MainWindow::setModeJTAG()
{
    if (!this->mStlink->isConnected())
        return;
    this->log("Changing mode to JTAG...");
    this->mStlink->setModeJTAG();
    usleep(100000);
    this->getMode();
}

void MainWindow::setModeSWD()
{
    if (!this->mStlink->isConnected())
        return;
    this->log("Changing mode to SWD...");
    this->mStlink->setModeSWD();
    usleep(100000);
    this->getMode();
}

void MainWindow::Quit()
{
    this->hide();
    if (this->mStlink->isConnected())
        this->Disconnect();
    qApp->quit();
}

void MainWindow::getVersion()
{
    this->log("Fetching version...");
    this->mStlink->getVersion();
}

void MainWindow::getMode()
{
    this->log("Fetching mode...");
    const quint8 mode = this->mStlink->getMode();
    QString mode_str;
    switch (mode) {
        case STLink::Mode::UNKNOWN:
            mode_str = "Unknown";
            break;
        case STLink::Mode::DFU:
            mode_str = "DFU";
            break;
        case STLink::Mode::MASS:
            mode_str = "Mass Storage";
            break;
        case STLink::Mode::DEBUG:
            mode_str = "Debug";
            break;
        default:
            mode_str = "Unknown";
            break;
        }
        this->log("Mode: "+mode_str);
}

void MainWindow::getStatus()
{
    this->log("Fetching status...");
    const quint8 status = this->mStlink->getStatus();
    QString status_str;
    switch (status) {
        case STLink::Status::CORE_RUNNING:
            status_str = "Core Running";
            break;
        case STLink::Status::CORE_HALTED:
            status_str = "Core Halted";
            break;
        default:
            status_str = "Unknown";
            break;
        }
        this->log("Status: "+status_str);
}

bool MainWindow::getMCU()
{
    this->log("Fetching MCU Info...");
    this->mStlink->getCoreID();
    this->mStlink->resetMCU();
    this->mStlink->getChipID();

    if (this->mDevices->search(this->mStlink->mChipId)) {
        this->mStlink->mDevice = this->mDevices->mCurDevice;
        qInformal() << "Device type: " << this->mStlink->mDevice->mType;

        this->mUi->le_type->setText(this->mStlink->mDevice->mType);
        this->mUi->le_chipid->setText("0x"+QString::number((*this->mStlink->mDevice)["chip_id"], 16));
        this->mUi->le_flashbase->setText("0x"+QString::number((*this->mStlink->mDevice)["flash_base"], 16));
        //this->ui->le_flashsize->setText(QString::number((*this->stlink->device)["flash_size"]/1024)+"KB");
        this->mUi->le_ramsize->setText(QString::number((*this->mStlink->mDevice)["sram_size"]/1024)+"KB");
        this->mUi->le_rambase->setText("0x"+QString::number((*this->mStlink->mDevice)["sram_base"], 16));

        this->mUi->le_stlver->setText(QString::number(this->mStlink->mVersion.stlink));
        this->mUi->le_jtagver->setText(QString::number(this->mStlink->mVersion.jtag));
        this->mUi->le_swimver->setText(QString::number(this->mStlink->mVersion.swim));

        if(!this->mStlink->mVersion.stlink)
            this->mUi->le_jtagver->setToolTip("Not supported");
        if(!this->mStlink->mVersion.swim)
            this->mUi->le_swimver->setToolTip("Not supported");

        (*this->mStlink->mDevice)["flash_size"] = this->mStlink->readFlashSize();
        this->mUi->le_flashsize->setText(QString::number((*this->mStlink->mDevice)["flash_size"])+"KB");

        return true;
    }
    this->log("Device not found in database!");
    qCritical() << "Device not found in database!";
    return false;
}
