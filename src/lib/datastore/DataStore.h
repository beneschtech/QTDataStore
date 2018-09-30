#ifndef DATASTORE_H
#define DATASTORE_H
#include <QObject>
#include <lmdb.h>
#include <openssl/sha.h>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QDir>

class DataStore: public QObject
{
    Q_OBJECT

public:
    static DataStore* instance();
    static void init(QString path,QStringList subDbs = QStringList());
    virtual QString filePath(void *metaData,size_t metaDataSize);

private:
    QDir myDbPath;
    QStringList mySubDBs;

    // Init / creation variables
    Q_DISABLE_COPY(DataStore)
    DataStore(QString &,QStringList &);
    static DataStore *myInstance;
    static QMutex myMutex;
};

#endif // DATASTORE_H
