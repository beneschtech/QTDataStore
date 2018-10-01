#include "storetest.h"

void StoreTest::insertTest()
{
    std::cout << "\nFile I/O tests\n" << std::endl;
    double start = currentTime();
    int rv;
    DataStore::instance()->insert("TESTKEY","TESTVALOK");
    for (int i = 0;i< 999; i++)
    {
        QString key = QString::asprintf("%016ld",random() % 10000);
        QByteArray val = "You will rue the day!! Well... start rueing!";
        rv = DataStore::instance()->insert(key,val);
        if (rv != 0)
        {
            break;
        }
    }
    if (rv == 0)
    {
        std::cout << "1000 unordered inserts: " << std::fixed << (currentTime() - start)*1000 << "ms" << std::endl;
    }
    QCOMPARE(rv,0);
}

void StoreTest::findTest()
{
    QByteArray tval;
    double start = currentTime();
    int rv = DataStore::instance()->find("TESTKEY",tval);
    QCOMPARE(tval.data(),"TESTVALOK");
    QCOMPARE(rv,0);
    int matches = 1;
    for (int i = 0;i< 999; i++)
    {
        QString key = QString::asprintf("%016ld",random() % 10000);
        QByteArray val = "";
        rv = DataStore::instance()->find(key,val);
        if (rv != 0 && rv != MDB_NOTFOUND)
        {
            break;
        }
        if (rv == 0)
            matches++;
    }
    if (rv == 0 || rv == MDB_NOTFOUND)
    {
        rv = 0;
        std::cout << "1000 random reads: " << std::fixed << (currentTime() - start)*1000 << "ms" << " " << matches << " matches" << std::endl;
    }

    QCOMPARE(rv,0);
}

void StoreTest::delTest()
{
    QByteArray tval;
    int rv = DataStore::instance()->remove("TESTKEY");
    QCOMPARE(rv,0);
    rv = DataStore::instance()->find("TESTKEY",tval);
    QCOMPARE(tval.size(),0);
    QCOMPARE(rv,MDB_NOTFOUND);
}

void StoreTest::gtTest()
{
    QMap<QString,QByteArray> ra;
    int rv = DataStore::instance()->greaterThan("0000000000007000",ra);
    QCOMPARE(rv,0);
    std::cout << "Items with keys > 7000: " << ra.size() << std::endl;
}
