#ifndef DATASTORE_H
#define DATASTORE_H
#include <QObject>
#include <lmdb.h>
#include <openssl/sha.h>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QDateTime>
#include <QDir>
#include <QMap>

class DataStore: public QObject
{
    Q_OBJECT

public:
    static DataStore* instance();
    static void init(QString path,QStringList subDbs = QStringList());
    void shutdown();

    // Getter/Setters
    void setDataDir(QString s) { myDataDir = s; }
    QString &dataDir() { return myDataDir; }

    // Derived object overrides
    virtual QString filePath(void *metaData,size_t metaDataSize);
    virtual bool openDatabase();
    virtual int insert(QString key,QByteArray data,QString subDb = QString());
    virtual int find(QString key,QByteArray &data,QString subDb = QString());
    virtual int remove(QString key,QString subDb = QString());
    virtual QString dateTimeToStr(QDateTime);
    virtual QString dateTimeStrUnique(QDateTime,QString subDb = QString());
    virtual int greaterThan(QString key,QMap<QString,QByteArray> &,QString subDb = QString());
    virtual int lessThan(QString key,QMap<QString,QByteArray> &,QString subDb = QString());
    virtual int all(QMap<QString,QByteArray> &,QString subDb = QString());

    // The below function is the most important to be able to search on more than just the key
    virtual bool filter(QString &,QByteArray &) { return true; }

private:
    QDir myDbPath;
    QStringList mySubDBs;
    QString myDataDir;
    MDB_env *myMDBEnv;
    QMap<QString,MDB_dbi> myDBHandles;

    // Init / creation variables
    Q_DISABLE_COPY(DataStore)
    DataStore(QString &,QStringList &);
    static DataStore *myInstance;
    static QMutex myMutex;
};

#endif // DATASTORE_H
