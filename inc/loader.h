#ifndef LOADER_H
#define LOADER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <compat.h>

namespace Loader {

    namespace Addr {

        const quint32 SRAM = 0x20000000;
        const quint32 PARAMS = 0x200007D0;
        const quint32 OFFSET_DEST = 0x00;
        const quint32 OFFSET_LEN = 0x04;
        const quint32 OFFSET_STATUS = 0x08;
        const quint32 OFFSET_POS = 0x0C;
        const quint32 OFFSET_TEST = 0x10;
        const quint32 BUFFER = 0x20000800;
    }
    namespace Masks {

        const quint32 STRT = (1<<0);
        const quint32 BUSY = (1<<1);
        const quint32 SUCCESS = (1<<2);
        const quint32 DEL = (1<<3);
        const quint32 VEREN = (1<<4);
        const quint32 ERR = (1<<15);
    }

}

class LoaderData : public QObject
{
    Q_OBJECT
public:
    explicit LoaderData(QObject *parent = 0);
    bool loadBin(QString const &path);
    QByteArray& refData(void);
    
signals:
    
public slots:

private:
    QByteArray mData;
    
};

#endif // LOADER_H
