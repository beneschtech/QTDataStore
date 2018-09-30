#include <iostream>
#include <QString>
#include <QtTest>
#include <DataStore.h>

class StoreTest : public QObject
{
    Q_OBJECT

public:
    StoreTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void hashTest();
};

StoreTest::StoreTest()
{
}

void StoreTest::initTestCase()
{
    DataStore::init("/tmp/datastoretest");
    QVERIFY(DataStore::instance());
}

void StoreTest::cleanupTestCase()
{
    QDir d("/tmp/datastoretest");
    QVERIFY(d.removeRecursively());
}

void StoreTest::hashTest()
{
    std::string metaData = "Now is the winter of your discontent!";
    QString mockFname = DataStore::instance()->filePath((void *)metaData.c_str(),metaData.length());
    QCOMPARE(mockFname,"/tmp/datastoretest/e0/e9/56c3ec53728b2b5cd4ec7fb7aa63151f407688bfdcb3c21735979c122b");
}

QTEST_APPLESS_MAIN(StoreTest)

#include "tst_storetest.moc"
