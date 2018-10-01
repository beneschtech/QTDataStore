#include "storetest.h"

void StoreTest::hashTest()
{
    std::cout << "\n\nLogic Tests" << std::endl;
    std::string metaData = "Now is the winter of your discontent!";
    double start = currentTime();
    QString mockFname = DataStore::instance()->filePath((void *)metaData.c_str(),metaData.length());
    QCOMPARE(mockFname,QString("/tmp/datastoretest/data/e0/e9/56c3ec53728b2b5cd4ec7fb7aa63151f407688bfdcb3c21735979c122b"));
    for (int i = 0; i < 1000; i++)
    {
        QString md = QString::asprintf("%016ld%016ld",random(),random());
        std::string s = md.toStdString();
        DataStore::instance()->filePath((void *)s.c_str(),s.length());
    }
    std::cout << "1000 sha256 hashes: " << std::fixed << (currentTime() - start)*1000 << "ms" << std::endl;
}

void StoreTest::dateTimeTest()
{
    QDateTime dt;
    dt = dt.toUTC();
    dt.setMSecsSinceEpoch(10000000);
    QString key = DataStore::instance()->dateTimeToStr(dt);
    QCOMPARE(key,QString("0000000010000000"));
}

void StoreTest::dateTimeUniqueTest()
{
    QDateTime dt = QDateTime::currentDateTime();
    QString key1 = DataStore::instance()->dateTimeToStr(dt);
    int rv = DataStore::instance()->insert(key1,"some data");
    QCOMPARE(rv,0);
    QString key2 = DataStore::instance()->dateTimeStrUnique(dt);
    QVERIFY(key1 != key2);
}
