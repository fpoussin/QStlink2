#include "loader.h"

loader::loader(QObject *parent) :
    QObject(parent)
{




}

bool loader::loadBin(const QString &path) {

    QFile file(path);

    if (!file.open(QFile::ReadOnly))
        return false;

    this->m_data = file.readAll();
    file.close();

    return true;
}

bool loader::loadToRam(quint32 addr) {


    return true;
}
