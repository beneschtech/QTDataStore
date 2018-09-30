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

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void hashTest();
};

#endif // STORETEST_H
