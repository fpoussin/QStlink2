#include "loader.h"

loader::loader(QObject *parent) :
    QObject(parent)
{




}

bool loader::loadBin(const QString &path) {

    const QString _path = ":/bin/"+path;
    qInformal() << "Loader" << _path;

    QFile file(_path);

    if (!file.open(QFile::ReadOnly))
        return false;

    this->m_data = file.readAll();
    file.close();

    return true;
}

QByteArray& loader::refData(void) {

    return this->m_data;
}
