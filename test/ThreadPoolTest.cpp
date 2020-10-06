#include <chrono>
#include <iostream>
#include <vector>

#include "../third-party/ThreadPool.h"

#include "catch2/catch.hpp"

int test_ThreadPool()
{

    ThreadPool pool(4);
    std::vector<std::future<int>> results;

    for (int i = 0; i < 8; ++i)
    {
        results.emplace_back(pool.enqueue([i] {
            std::cout << "hello " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "world " << i << std::endl;
            return i * i;
        }));
    }

    for (auto&& result : results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;

    return 0;
}

TEST_CASE("test parallel accessor", "test ThreadPool")
{
    REQUIRE(test_ThreadPool() == 0);
}
