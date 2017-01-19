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
    mUi->b_disconnect->setEnabled(false);
    this->lockUI(true);
    mStlink = new stlinkv2();
    mDevices = new DeviceInfoList(this);
    mTfThread = new transferThread();

    mLastAction = ACTION_NONE;

    if (mDevices->IsLoaded()) {

        this->log(QString::number(mDevices->getDevicesCount())+" Device descriptions loaded.");
        QObject::connect(mUi->b_quit,SIGNAL(clicked()),this,SLOT(quit()));
        QObject::connect(mUi->b_connect, SIGNAL(clicked()), this, SLOT(connect()));
        QObject::connect(mUi->b_disconnect, SIGNAL(clicked()), this, SLOT(disconnect()));
        QObject::connect(mUi->b_send, SIGNAL(clicked()), this, SLOT(send()));
        QObject::connect(mUi->b_receive, SIGNAL(clicked()), this, SLOT(receive()));
        QObject::connect(mUi->b_verify, SIGNAL(clicked()), this, SLOT(verify()));
        QObject::connect(mUi->b_repeat, SIGNAL(clicked()), this, SLOT(repeat()));
        QObject::connect(mUi->b_halt, SIGNAL(clicked()), this, SLOT(haltMCU()));
        QObject::connect(mUi->b_run, SIGNAL(clicked()), this, SLOT(runMCU()));
        QObject::connect(mUi->b_reset, SIGNAL(clicked()), this, SLOT(resetMCU()));
        QObject::connect(mUi->r_jtag, SIGNAL(clicked()), this, SLOT(setModeJTAG()));
        QObject::connect(mUi->r_swd, SIGNAL(clicked()), this, SLOT(setModeSWD()));
        QObject::connect(mUi->b_hardReset, SIGNAL(clicked()), this, SLOT(hardReset()));

        // Thread
        QObject::connect(mTfThread, SIGNAL(sendProgress(quint32)), this, SLOT(updateProgress(quint32)));
        QObject::connect(mTfThread, SIGNAL(sendStatus(QString)), this, SLOT(updateStatus(QString)));
        QObject::connect(mTfThread, SIGNAL(sendLoaderStatus(QString)), this, SLOT(updateLoaderStatus(QString)));
        QObject::connect(mStlink, SIGNAL(bufferPct(quint32)), this, SLOT(updateLoaderPct(quint32)));
        QObject::connect(mTfThread, SIGNAL(sendLock(bool)), this, SLOT(lockUI(bool)));
        QObject::connect(mUi->b_stop, SIGNAL(clicked()), mTfThread, SLOT(halt()));
        QObject::connect(mTfThread, SIGNAL(sendLog(QString)), this, SLOT(log(QString)));

        // Help
        QObject::connect(mUi->b_help, SIGNAL(clicked()), this, SLOT(showHelp()));

        // Usb stuff
        QObject::connect(mStlink, SIGNAL(deviceDetected(QString)), this, SLOT(log(QString)));
    }

    else {
        this->log("Could not load the devices list");
    }
}

MainWindow::~MainWindow()
{
    mTfThread->exit();
    delete mTfThread;
    delete mStlink;
    delete mDevices;
    delete mUi;
}

void MainWindow::showHelp()
{

    mDialog.setText("Help","Could no load help file");

    QFile file(":/help.html");
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the help file.");
    }

    mDialog.setHTML(QString("Help"), QString(file.readAll()));
    mDialog.show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    (void)event;
    this->quit();
}

bool MainWindow::connect()
{
    PrintFuncName();
    this->log("Searching Device...");

    /* Try STL V2 first */
    mStlink->setSTLinkIDs();
    qint32 ret = mStlink->connect();

    /* Try Nucleo */
    if (ret < 0) {
        mStlink->setNucleoIDs();
        ret = mStlink->connect();
    }

    if (ret < 0) {
        this->log("ST Link V2 / Nucleo not found or unable to access it.");
#if defined(QWINUSB) && defined(WIN32)
        this->log("Did you install the official ST-Link V2 / Nucleo driver ?");
#elif !defined(WIN32)
        this->log("Did you install the udev rules ?");
#endif
        this->log("USB error: "+QString::number(ret));
        return false;
    }

    else {
        this->log("ST Link V2 / Nucleo found!");
        this->getVersion();
        this->getMode();

        mStlink->setExitModeDFU();
        if (mUi->r_jtag->isChecked())
            this->setModeJTAG();
        else
            this->setModeSWD();
        //this->hardReset();
        this->getStatus();
        if (this->getMCU()) {
            this->lockUI(false);
            return true;
        }
        else {
            this->disconnect();
            return false;
        }
    }
    return false;
}

void MainWindow::disconnect()
{
    this->log("Disconnecting...");
    mStlink->disconnect();
    this->log("Disconnected.");
    this->lockUI(true);
}

void MainWindow::log(const QString &s)
{
    mUi->t_log->appendPlainText(s);
    qInfo(s.toStdString().c_str());
}

void MainWindow::lockUI(bool enabled)
{
    mUi->gb_top->setEnabled(!enabled);
    mUi->gb_bottom->setEnabled(!enabled);

    mUi->b_connect->setEnabled(enabled);
    mUi->b_disconnect->setEnabled(!enabled);
    mUi->gb_bottom->setEnabled(!enabled);
    mUi->b_send->setEnabled(!enabled);
    mUi->b_receive->setEnabled(!enabled);
    mUi->b_verify->setEnabled(!enabled);
    mUi->b_repeat->setEnabled(!enabled);
    mUi->b_halt->setEnabled(!enabled);
    mUi->b_reset->setEnabled(!enabled);
    mUi->b_run->setEnabled(!enabled);
    mUi->b_hardReset->setEnabled(!enabled);
}

void MainWindow::updateProgress(quint32 p)
{
    mUi->pgb_transfer->setValue(p);
}

void MainWindow::updateStatus(const QString &s)
{
    mUi->l_progress->setText(s);
}

void MainWindow::updateLoaderStatus(const QString &s)
{
    mUi->l_status->setText(s);
}

void MainWindow::updateLoaderPct(quint32 p)
{
    mUi->pgb_loader->setValue(p);
}

void MainWindow::send()
{
    mFilename.clear();
    mFilename = QFileDialog::getOpenFileName(this, "Open file", "", "Binary Files (*.bin)");
    if (!mFilename.isNull()) {
        QFile file(mFilename);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Could not open the file.");
            return;
        }
        this->log("Size: "+QString::number(file.size()/1024)+"KB");

        if (file.size() > mStlink->mDevice->value("flash_size")*1024) {
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

        this->send(mFilename);
        mLastAction = ACTION_SEND;
    }
}

void MainWindow::send(const QString &path)
{
    qDebug("Writing flash");
    this->log("Sending "+path);
    mStlink->resetMCU(); // We stop the MCU
    mUi->tabw_info->setCurrentIndex(3);
    mUi->pgb_transfer->setValue(0);
    mUi->l_progress->setText("Starting transfer...");

    // Transfer thread
    mTfThread->setParams(mStlink, path, true, false);
    mTfThread->start();
}

void MainWindow::receive()
{
    qDebug("Reading flash");
    mFilename.clear();
    mFilename = QFileDialog::getSaveFileName(this, "Save File", "", "Binary Files (*.bin)");
    if (!mFilename.isNull()) {
        QFile file(mFilename);
        if (!file.open(QIODevice::ReadWrite)) {
            qCritical("Could not save the file.");
            return;
        }
        file.close();
        this->receive(mFilename);
        mLastAction = ACTION_RECEIVE;
    }
}

void MainWindow::receive(const QString &path)
{
    this->log("Saving to "+path);
    mUi->tabw_info->setCurrentIndex(3);
    mUi->pgb_transfer->setValue(0);
    mUi->l_progress->setText("Starting transfer...");

    // Transfer thread
    mTfThread->setParams(mStlink, path, false, false);
    mTfThread->start();
}

void MainWindow::verify()
{
    qDebug("Verify flash");
    mFilename.clear();
    mFilename = QFileDialog::getOpenFileName(this, "Open file", "", "Binary Files (*.bin)");
    if (!mFilename.isNull()) {
        QFile file(mFilename);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Could not open the file.");
            return;
        }
        file.close();
        this->verify(mFilename);
        mLastAction = ACTION_VERIFY;
    }
}

void MainWindow::repeat()
{
    switch (mLastAction) {

        case ACTION_SEND:
            this->send(mFilename);
            break;
        case ACTION_RECEIVE:
            this->receive(mFilename);
            break;
        case ACTION_VERIFY:
            this->verify(mFilename);
            break;
        case ACTION_NONE:
            this->log("Nothing to repeat.");
            break;
        default:
            break;
    }
}

void MainWindow::verify(const QString &path)
{
    this->log("Verifying "+path);
    mUi->tabw_info->setCurrentIndex(3);
    mUi->pgb_transfer->setValue(0);
    mUi->l_progress->setText("Starting Verification...");

    // Transfer thread
    mTfThread->setParams(mStlink, path, false, true);
    mTfThread->start();
}

void MainWindow::eraseFlash()
{
    mStlink->hardResetMCU();
    mStlink->resetMCU();
    if (!mStlink->unlockFlash())
        return;
    mStlink->eraseFlash();
}

void MainWindow::haltMCU()
{
    this->log("Halting MCU...");
    mStlink->haltMCU();
    SleepThread::msleep(100);
    this->getStatus();
}

void MainWindow::runMCU()
{
    this->log("Resuming MCU...");
    mStlink->runMCU();
    SleepThread::msleep(100);
    this->getStatus();
}

void MainWindow::resetMCU()
{
    this->log("Reseting MCU...");
    mStlink->resetMCU();
    SleepThread::msleep(100);
    this->getStatus();
}

void MainWindow::hardReset()
{
    this->log("Hard Reset...");
    mStlink->hardResetMCU();
    SleepThread::msleep(100);
    this->getStatus();
}

void MainWindow::setModeJTAG()
{
    if (!mStlink->isConnected())
        return;
    this->log("Changing mode to JTAG...");
    mStlink->setModeJTAG();
    SleepThread::msleep(100);
    this->getMode();
}

void MainWindow::setModeSWD()
{
    if (!mStlink->isConnected())
        return;
    this->log("Changing mode to SWD...");
    mStlink->setModeSWD();
    SleepThread::msleep(100);
    this->getMode();
}

void MainWindow::quit()
{
    this->hide();
    if (mStlink->isConnected())
        this->disconnect();
    qApp->quit();
}

void MainWindow::getVersion()
{
    this->log("Fetching version...");
    stlinkv2::STVersion v(mStlink->getVersion());

    this->mUi->le_stlver->setText(QString::number(v.stlink));
    this->mUi->le_jtagver->setText(QString::number(v.jtag));
    this->mUi->le_swimver->setText(QString::number(v.swim));
}

void MainWindow::getMode()
{
    this->log("Fetching mode...");
    const quint8 mode = mStlink->getMode();
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
    const quint8 status = mStlink->getStatus();
    QString status_str;
    switch (status) {
        case STLink::Status::RUNNING:
            status_str = "Core Running";
            break;
        case STLink::Status::HALTED:
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
    mStlink->getCoreID();
    mStlink->resetMCU();
    mStlink->getChipID();

    if (mDevices->search(mStlink->mChipId)) {
        mStlink->mDevice = mDevices->mCurDevice;
        qInfo() << "Device type: " << mStlink->mDevice->mType;

        mUi->le_type->setText(mStlink->mDevice->mType);
        mUi->le_chipid->setText("0x"+QString::number(mStlink->mDevice->value("chip_id"), 16));
        mUi->le_flashbase->setText("0x"+QString::number(mStlink->mDevice->value("flash_base"), 16));
        //this->ui->le_flashsize->setText(QString::number((*this->stlink->device)["flash_size"]/1024)+"KB");

        mUi->le_stlver->setText(QString::number(mStlink->mVersion.stlink));
        mUi->le_jtagver->setText(QString::number(mStlink->mVersion.jtag));
        mUi->le_swimver->setText(QString::number(mStlink->mVersion.swim));

        if(!mStlink->mVersion.stlink)
            mUi->le_jtagver->setToolTip("Not supported");
        if(!mStlink->mVersion.swim)
            mUi->le_swimver->setToolTip("Not supported");

        mStlink->mDevice->insert("flash_size", mStlink->readFlashSize());
        mUi->le_flashsize->setText(QString::number(mStlink->mDevice->value("flash_size"))+"KB");

        return true;
    }
    this->log("Device not found in database!");
    qCritical("Device not found in database!");
    return false;
}
