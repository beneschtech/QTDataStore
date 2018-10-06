#ifndef STORETEST_H
#define STORETEST_H

#include <iostream>
#include <QString>
#include <QtTest>
#include <QTemporaryDir>
#include <DataStore.h>

class StoreTest : public QObject
{
    Q_OBJECT

public:
    StoreTest();

private:
    double currentTime();
    QTemporaryDir myTestDir;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // IO tests
    void insertTest();
    void findTest();
    void delTest();
    void gtTest();
    void ltTest();
    void allTest();

    // Logic tests
    void hashTest();
    void dateTimeTest();
    void dateTimeUniqueTest();
};

#ifdef __MINGW64__
#define srandom(x) srand((unsigned int)x)
#define random(x) (long)rand(x)
#endif
#endif // STORETEST_H
