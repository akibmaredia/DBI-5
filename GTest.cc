#include "DBFile.h"
#include "Statistics.h"
#include <gtest/gtest.h>
#include "MainScheme.h"


TEST(MainScheme, initializeSchemaMap) {
    SchemeStore schemeStore;
    MainScheme mainScheme;
    mainScheme.initializeSchemeStore(schemeStore);
    ASSERT_TRUE(schemeStore.find("nation") != schemeStore.end());
}

TEST(MainScheme, initializeStatistics) {
    Statistics statistics;
    MainScheme mainScheme;
    mainScheme.initializeStatistics(statistics);
    ASSERT_TRUE(statistics.Exists("nation"));
    ASSERT_FALSE(statistics.Exists("n"));
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}