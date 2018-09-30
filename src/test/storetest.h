#ifndef STORETEST_H
#define STORETEST_H

#include <iostream>
#include <QString>
#include <QtTest>
#include <DataStore.h>

class StoreTest : public QObject
{
    Q_OBJECT

public:
    StoreTest();

private:
    double currentTime();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // Actual tests
    void hashTest();
    void insertTest();
    void findTest();
    void delTest();
    void dateTimeTest();
    void dateTimeUniqueTest();
};

#endif // STORETEST_H
