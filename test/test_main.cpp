/**
 * Boilerplate test code to initialise framework.
 */

#if PPP_USE_GTEST
#include "gmock/gmock.h"
#include "gtest/gtest.h"


int main(int argc, char* argv[])
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
#else

#define USE_CATCH2 1
#if USE_CATCH2
/// CATCH_CONFIG_MAIN should be put before `#include "catch2/catch.hpp"ls`
#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#endif

#endif