#include <QMutexLocker>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "DataStore.h"

DataStore *DataStore::myInstance = nullptr;
QMutex DataStore::myMutex;

void DataStoreAtExit()
{
    // Attempts to close everything if the program abends and not leave stray locks
    if (DataStore::instance())
        DataStore::instance()->shutdown();
}

DataStore::DataStore(QString &path, QStringList &subdirs):
    myDataDir(""),
    myMDBEnv(nullptr)
{
    if (myInstance)
    {
        std::cerr << "DataStore already initialized" << std::endl;
        return;
    } else {
        myInstance = this;
    }
    myDbPath.setPath(path);
    mySubDBs = subdirs;
    if (!myDbPath.exists())
    {
        if (!myDbPath.mkpath(path))
        {
            std::cerr << "Could not create directory: " << path.toStdString() << std::endl;
            myInstance = nullptr;
            return;
        }        
    }    
    if (!openDatabase())
    {
        myMDBEnv = nullptr;
        return;
    }
    atexit(DataStoreAtExit);
}

DataStore *DataStore::instance()
{
    if (myInstance)
    {
        if (myMutex.try_lock())
        {
            myMutex.unlock();
            return myInstance;
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

void DataStore::init(QString path, QStringList subDbs)
{
    QMutexLocker l(&myMutex);
    new DataStore(path,subDbs);
}

void DataStore::shutdown()
{
    QMap<QString,MDB_dbi>::iterator dit = myDBHandles.begin();
    while (dit != myDBHandles.end())
    {
        mdb_dbi_close(myMDBEnv,dit.value());
        dit++;
    }
    mdb_env_close(myMDBEnv);
    myInstance = nullptr;
}

bool DataStore::openDatabase()
{
    int rval;
    if ((rval = mdb_env_create(&myMDBEnv)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return false;
    }
    if (mySubDBs.size())
    {
        if ((rval = mdb_env_set_maxdbs(myMDBEnv,(MDB_dbi)mySubDBs.size())))
        {
            std::cerr << mdb_strerror(rval) << std::endl;
            return false;
        }
    }
    mdb_mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if ((rval = mdb_env_open(myMDBEnv,myDbPath.absolutePath().toStdString().c_str(),MDB_NOTLS,perms)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return false;
    }
    if (mySubDBs.size())
    {
        int i = 0;
        while (i < mySubDBs.length())
        {
            const char *dbname = mySubDBs[i].toStdString().c_str();
            MDB_txn *txn;
            if ((rval = mdb_txn_begin(myMDBEnv,nullptr,0,&txn)) != 0)
            {
                std::cerr << mdb_strerror(rval) << std::endl;
                mdb_env_close(myMDBEnv);
                return false;
            }
            MDB_dbi dbh;
            if ((rval = mdb_dbi_open(txn,dbname,MDB_CREATE,&dbh)) != 0)
            {
                std::cerr << mdb_strerror(rval) << std::endl;
                mdb_txn_abort(txn);
                mdb_env_close(myMDBEnv);
                return false;
            }
            myDBHandles[dbname] = dbh;
            mdb_txn_commit(txn);
            i++;
        }
    } else {
        MDB_txn *txn;
        if ((rval = mdb_txn_begin(myMDBEnv,nullptr,0,&txn)) != 0)
        {
            std::cerr << mdb_strerror(rval) << std::endl;
            mdb_env_close(myMDBEnv);
            return false;
        }
        MDB_dbi dbh;
        if ((rval = mdb_dbi_open(txn,nullptr,MDB_CREATE,&dbh)) != 0)
        {
            std::cerr << mdb_strerror(rval) << std::endl;
            mdb_txn_abort(txn);
            mdb_env_close(myMDBEnv);
            return false;
        }
        myDBHandles[QString()] = dbh;
        mdb_txn_commit(txn);
    }
    return true;
}

QString DataStore::filePath(void *metaData, size_t metaDataSize)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char hexString[(SHA256_DIGEST_LENGTH * 2) + 1];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, metaData, metaDataSize);
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
       sprintf(hexString + (i * 2), "%02x", hash[i]);
    }
    hexString[64] = 0;

    QString rv = myDbPath.absolutePath();
    rv.append(QDir::separator());
    hexString[2] = hexString[5] = QDir::separator().toLatin1();
    if (myDataDir.length())
    {
        rv.append(myDataDir);
        rv.append(QDir::separator());
    }
    rv.append(hexString);
    return rv;
}

QString DataStore::dateTimeToStr(QDateTime dt)
{
    qint64 mSecs = dt.toUTC().toMSecsSinceEpoch();
    mSecs *= 1000;
    return QString::asprintf("%016lld",mSecs);
}

QString DataStore::dateTimeStrUnique(QDateTime dt, QString subDb)
{
    qint64 mSecs = dt.toUTC().toMSecsSinceEpoch();
    mSecs *= 1000;
    QByteArray ba;
    QString key = QString::asprintf("%016lld",mSecs);
    int rv = find(key,ba,subDb);
    while (rv != MDB_NOTFOUND)
    {
        mSecs++;
        key = QString::asprintf("%016lld",mSecs);
        rv = find(key,ba,subDb);
    }
    return key;
}

int DataStore::find(QString key, QByteArray &data, QString subDb)
{
    int rval;
    MDB_txn *txn;
    if ((rval = mdb_txn_begin(myMDBEnv,nullptr,MDB_RDONLY,&txn)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return -1;
    }
    MDB_dbi dbh;
    QMap<QString,MDB_dbi>::iterator dbi = myDBHandles.find(subDb);
    if (dbi == myDBHandles.end())
    {
        mdb_txn_commit(txn);
        return -1;
    }
    dbh = dbi.value();
    MDB_val mkey,mdata;
    std::string s = key.toStdString();
    mkey.mv_data = (void *)s.c_str();
    mkey.mv_size = s.length();
    data.clear();
    rval = mdb_get(txn,dbh,&mkey,&mdata);
    if (rval != 0)
    {
        if (rval != MDB_NOTFOUND)
            std::cerr << mdb_strerror(rval) << std::endl;
    } else {
        data.insert(0,(const char *)mdata.mv_data,mdata.mv_size);
    }
    mdb_txn_abort(txn);
    return rval;

}

int DataStore::remove(QString key,QString subDb)
{
    int rval;
    MDB_txn *txn;
    if ((rval = mdb_txn_begin(myMDBEnv,nullptr,0,&txn)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return -1;
    }
    MDB_dbi dbh;
    QMap<QString,MDB_dbi>::iterator dbi = myDBHandles.find(subDb);
    if (dbi == myDBHandles.end())
    {
        mdb_txn_commit(txn);
        return -1;
    }
    dbh = dbi.value();
    MDB_val mkey;
    std::string s = key.toStdString();
    mkey.mv_data = (void *)s.c_str();
    mkey.mv_size = s.length();

    rval = mdb_del(txn,dbh,&mkey,nullptr);
    if (rval != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
    }
    mdb_txn_commit(txn);
    return rval;

}

int DataStore::insert(QString key, QByteArray data, QString subDb)
{
    int rval;
    MDB_txn *txn;
    if ((rval = mdb_txn_begin(myMDBEnv,nullptr,0,&txn)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return -1;
    }
    MDB_dbi dbh;
    QMap<QString,MDB_dbi>::iterator dbi = myDBHandles.find(subDb);
    if (dbi == myDBHandles.end())
    {
        mdb_txn_commit(txn);
        return -1;
    }
    dbh = dbi.value();
    MDB_val mkey,mdata;
    std::string s = key.toStdString();
    mkey.mv_data = (void *)s.c_str();
    mkey.mv_size = s.length();
    mdata.mv_data = data.data();
    mdata.mv_size = data.size();
    rval = mdb_put(txn,dbh,&mkey,&mdata,MDB_NODUPDATA);
    if (rval != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
    }
    mdb_txn_commit(txn);
    return rval;
}

int DataStore::greaterThan(QString key, QMap<QString, QByteArray> &rarray, QString subDb)
{
    int rval;
    MDB_txn *txn;
    if ((rval = mdb_txn_begin(myMDBEnv,nullptr,MDB_RDONLY,&txn)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return rval;
    }
    MDB_dbi dbh;
    QMap<QString,MDB_dbi>::iterator dbi = myDBHandles.find(subDb);
    if (dbi == myDBHandles.end())
    {
        mdb_txn_commit(txn);
        return -1;
    }
    dbh = dbi.value();
    MDB_cursor *curs;
    if ((rval = mdb_cursor_open(txn,dbh,&curs)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return rval;
    }
    MDB_val mkey,mdata;
    std::string s = key.toStdString();
    mkey.mv_data = (void *)s.c_str();
    mkey.mv_size = s.length();
    if ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_SET_RANGE)) != 0)
    {
        if (rval != MDB_NOTFOUND) {
            std::cerr << mdb_strerror(rval) << std::endl;
            return rval;
        }
        mdb_cursor_close(curs);
        mdb_txn_commit(txn);
    }
    rarray.clear();

    if ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_GET_CURRENT)) != 0)
    {
        mdb_cursor_close(curs);
        mdb_txn_commit(txn);
        if (rval != MDB_NOTFOUND) {
            std::cerr << mdb_strerror(rval) << std::endl;
        }
        return rval;
    }
    QString rk = QByteArray::fromRawData((const char *)mkey.mv_data,mkey.mv_size);
    QByteArray rv = QByteArray::fromRawData((const char *)mdata.mv_data,mdata.mv_size);
    if (filter(rk,rv))
        rarray[rk] = rv;
    while ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_NEXT)) == 0)
    {
        rk = QByteArray::fromRawData((const char *)mkey.mv_data,mkey.mv_size);
        rv = QByteArray::fromRawData((const char *)mdata.mv_data,mdata.mv_size);
        if (filter(rk,rv))
            rarray[rk] = rv;
    }
    if (rval != MDB_NOTFOUND) {
        std::cerr << mdb_strerror(rval) << std::endl;
    }
    mdb_cursor_close(curs);
    mdb_txn_commit(txn);
    return 0;
}

int DataStore::lessThan(QString key, QMap<QString, QByteArray> &rarray, QString subDb)
{
    int rval;
    MDB_txn *txn;
    if ((rval = mdb_txn_begin(myMDBEnv,nullptr,MDB_RDONLY,&txn)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return rval;
    }
    MDB_dbi dbh;
    QMap<QString,MDB_dbi>::iterator dbi = myDBHandles.find(subDb);
    if (dbi == myDBHandles.end())
    {
        mdb_txn_commit(txn);
        return -1;
    }
    dbh = dbi.value();
    MDB_cursor *curs;
    if ((rval = mdb_cursor_open(txn,dbh,&curs)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return rval;
    }
    MDB_val mkey,mdata;
    std::string s = key.toStdString();
    mkey.mv_data = (void *)s.c_str();
    mkey.mv_size = s.length();
    if ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_SET_RANGE)) != 0)
    {
        if (rval != MDB_NOTFOUND) {
            std::cerr << mdb_strerror(rval) << std::endl;
            return rval;
        }
        mdb_cursor_close(curs);
        mdb_txn_commit(txn);
    }
    rarray.clear();

    if ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_GET_CURRENT)) != 0)
    {
        mdb_cursor_close(curs);
        mdb_txn_commit(txn);
        if (rval != MDB_NOTFOUND) {
            std::cerr << mdb_strerror(rval) << std::endl;
        }
        return rval;
    }
    QString rk = QByteArray::fromRawData((const char *)mkey.mv_data,mkey.mv_size);
    QByteArray rv = QByteArray::fromRawData((const char *)mdata.mv_data,mdata.mv_size);
    if (filter(rk,rv))
        rarray[rk] = rv;
    while ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_PREV)) == 0)
    {
        rk = QByteArray::fromRawData((const char *)mkey.mv_data,mkey.mv_size);
        rv = QByteArray::fromRawData((const char *)mdata.mv_data,mdata.mv_size);
        if (filter(rk,rv))
            rarray[rk] = rv;
    }
    if (rval != MDB_NOTFOUND) {
        std::cerr << mdb_strerror(rval) << std::endl;
    }
    mdb_cursor_close(curs);
    mdb_txn_commit(txn);
    return 0;
}

int DataStore::all(QMap<QString, QByteArray> &rarray, QString subDb)
{
    int rval;
    MDB_txn *txn;
    if ((rval = mdb_txn_begin(myMDBEnv,nullptr,MDB_RDONLY,&txn)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return rval;
    }
    MDB_dbi dbh;
    QMap<QString,MDB_dbi>::iterator dbi = myDBHandles.find(subDb);
    if (dbi == myDBHandles.end())
    {
        mdb_txn_commit(txn);
        return -1;
    }
    dbh = dbi.value();
    MDB_cursor *curs;
    if ((rval = mdb_cursor_open(txn,dbh,&curs)) != 0)
    {
        std::cerr << mdb_strerror(rval) << std::endl;
        return rval;
    }
    MDB_val mkey,mdata;
    rarray.clear();

    if ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_FIRST)) != 0)
    {
        mdb_cursor_close(curs);
        mdb_txn_commit(txn);
        if (rval != MDB_NOTFOUND) {
            std::cerr << mdb_strerror(rval) << std::endl;
        }
        return rval;
    }
    QString rk = QByteArray::fromRawData((const char *)mkey.mv_data,mkey.mv_size);
    QByteArray rv = QByteArray::fromRawData((const char *)mdata.mv_data,mdata.mv_size);
    if (filter(rk,rv))
        rarray[rk] = rv;
    while ((rval = mdb_cursor_get(curs,&mkey,&mdata,MDB_NEXT)) == 0)
    {
        rk = QByteArray::fromRawData((const char *)mkey.mv_data,mkey.mv_size);
        rv = QByteArray::fromRawData((const char *)mdata.mv_data,mdata.mv_size);
        if (filter(rk,rv))
            rarray[rk] = rv;
    }
    if (rval != MDB_NOTFOUND) {
        std::cerr << mdb_strerror(rval) << std::endl;
    }
    mdb_cursor_close(curs);
    mdb_txn_commit(txn);
    return 0;
}
