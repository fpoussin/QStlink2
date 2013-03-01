#ifndef LOADER_H
#define LOADER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QByteArray>

class loader : public QObject
{
    Q_OBJECT
public:
    explicit loader(QObject *parent = 0);
    bool loadBin(QString const &path);
    bool loadToRam(quint32 addr);
    
signals:
    
public slots:

private:
    QByteArray m_data;
    
};

#endif // LOADER_H
