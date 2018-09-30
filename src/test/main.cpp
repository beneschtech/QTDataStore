#include <sys/time.h>
#include "storetest.h"

StoreTest::StoreTest()
{
}

void StoreTest::initTestCase()
{
    QStringList dbs;
    DataStore::init("/tmp/datastoretest",dbs);
    QVERIFY(DataStore::instance());
    DataStore::instance()->setDataDir("data");
    QCOMPARE(DataStore::instance()->dataDir(),QString("data"));
}

void StoreTest::cleanupTestCase()
{
    QDir d("/tmp/datastoretest");
    QVERIFY(d.removeRecursively());
}

double StoreTest::currentTime()
{
    struct timeval tm = { 0,0 };
    gettimeofday(&tm,nullptr);
    double rv = tm.tv_sec;
    rv += (tm.tv_usec / 1000000.0);
    return rv;
}

QTEST_APPLESS_MAIN(StoreTest)
