#ifndef LOADER_H
#define LOADER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QByteArray>
#include <QDebug>
#include "compat.h"

namespace Loader {

namespace Addr {

const quint32 SRAM = 0x20000000; /**< TODO: describe */
const quint32 PARAMS = 0x200007D0; /**< TODO: describe */
const quint32 OFFSET_DEST = 0x00; /**< TODO: describe */
const quint32 OFFSET_LEN = 0x04; /**< TODO: describe */
const quint32 OFFSET_STATUS = 0x08; /**< TODO: describe */
const quint32 OFFSET_POS = 0x0C; /**< TODO: describe */
const quint32 OFFSET_TEST = 0x10; /**< TODO: describe */
const quint32 BUFFER = 0x20000800; /**< TODO: describe */
}
namespace Masks {

const quint32 STRT = (1 << 0); /**< TODO: describe */
const quint32 BUSY = (1 << 1); /**< TODO: describe */
const quint32 SUCCESS = (1 << 2); /**< TODO: describe */
const quint32 DEL = (1 << 3); /**< TODO: describe */
const quint32 VEREN = (1 << 4); /**< TODO: describe */
const quint32 ERR = (1 << 15); /**< TODO: describe */
}
}

/**
 * @brief
 *
 */
class LoaderData : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief
     *
     * @param parent
     */
    explicit LoaderData(QObject *parent = 0);
    /**
     * @brief
     *
     * @param path
     * @return bool
     */
    bool loadBin(QString const &path);
    /**
     * @brief
     *
     * @return QByteArray
     */
    QByteArray &refData(void);

signals:

public slots:

private:
    QByteArray mData; /**< TODO: describe */
};

#endif // LOADER_H
