#include "storetest.h"

StoreTest::StoreTest()
{
}

void StoreTest::initTestCase()
{
    DataStore::init("/tmp/datastoretest");
    QVERIFY(DataStore::instance());
    DataStore::instance()->setDataDir("data");
    QCOMPARE(DataStore::instance()->dataDir(),"data");
}

void StoreTest::cleanupTestCase()
{
    QDir d("/tmp/datastoretest");
    QVERIFY(d.removeRecursively());
}

QTEST_APPLESS_MAIN(StoreTest)
