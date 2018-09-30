#include <QMutexLocker>
#include <iostream>
#include "DataStore.h"

DataStore *DataStore::myInstance = nullptr;
QMutex DataStore::myMutex;

DataStore::DataStore(QString &path, QStringList &subdirs):
    myDataDir("")
{
    if (myInstance)
    {
        std::cerr << "DataStore already initialized" << std::endl;
        return;
    } else {
        myInstance = this;
    }
    myDbPath.setPath(path);
    if (!myDbPath.exists())
    {
        if (!myDbPath.mkpath(path))
        {
            std::cerr << "Could not create directory: " << path.toStdString() << std::endl;
            myInstance = nullptr;
            return;
        }
        // Create databases that dont exist yet
    }
    mySubDBs = subdirs;
    // Check if DBs are correct
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
