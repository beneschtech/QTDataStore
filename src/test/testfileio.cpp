#include "storetest.h"

void StoreTest::hashTest()
{
    std::string metaData = "Now is the winter of your discontent!";
    QString mockFname = DataStore::instance()->filePath((void *)metaData.c_str(),metaData.length());
    QCOMPARE(mockFname,"/tmp/datastoretest/data/e0/e9/56c3ec53728b2b5cd4ec7fb7aa63151f407688bfdcb3c21735979c122b");
}
