#include "loader.h"

LoaderData::LoaderData(QObject *parent) :
    QObject(parent)
{


}

bool LoaderData::loadBin(const QString &path) {

    const QString _path = ":/bin/"+path;
    qInfo() << "Loader" << _path;

    QFile file(_path);

    if (!file.open(QFile::ReadOnly))
        return false;

    mData = file.readAll();
    file.close();

    return true;
}

QByteArray& LoaderData::refData(void) {

    return mData;
}
