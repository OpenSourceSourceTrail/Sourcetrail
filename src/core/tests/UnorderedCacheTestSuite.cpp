#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "UnorderedCache.h"

using namespace ::testing;

// NOLINTNEXTLINE
TEST(UnorderedCache, goodCase) {
  uint32_t count = 0;
  UnorderedCache<int, int> cache([&count](int) {
    ++count;
    return 0;
  });
  EXPECT_EQ(0, cache.getValue(0));
  EXPECT_EQ(1, count);
  EXPECT_EQ(0, cache.getValue(0));
  EXPECT_EQ(1, count);
}

// Test Fixture for UnorderedCache
class UnorderedCacheTest : public ::testing::Test {
protected:
  // Helper function to create a simple calculator
  static int squareCalculator(const int& key) {
    return key * key;
  }

  // Helper function to simulate expensive computation
  static std::string expensiveStringComputation(const int& key) {
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return "Computed: " + std::to_string(key);
  }
};

// Basic functionality test
TEST_F(UnorderedCacheTest, BasicFunctionality) {
  UnorderedCache<int, int> cache(squareCalculator);

  // First call should be a miss
  EXPECT_EQ(cache.getValue(4), 16);

  // Second call should be a hit
  EXPECT_EQ(cache.getValue(4), 16);
}

// Test with lambda calculator
TEST_F(UnorderedCacheTest, LambdaCalculator) {
  UnorderedCache<int, std::string> cache([](const int& key) { return "Lambda: " + std::to_string(key); });

  EXPECT_EQ(cache.getValue(5), "Lambda: 5");
  EXPECT_EQ(cache.getValue(5), "Lambda: 5");
}

// Concurrency and performance test
TEST_F(UnorderedCacheTest, ConcurrencyTest) {
  UnorderedCache<int, std::string> cache(expensiveStringComputation);

  // Simulate multiple threads accessing the cache
  std::vector<std::thread> threads;
  std::vector<std::string> results(10);

  for(size_t i = 0; i < 10; ++i) {
    threads.emplace_back([&cache, &results, i]() { results[i] = cache.getValue(static_cast<int>(i)); });
  }

  for(auto& t : threads) {
    t.join();
  }

  // Verify all results are correct
  for(size_t i = 0; i < 10; ++i) {
    EXPECT_EQ(results[i], "Computed: " + std::to_string(i));
  }
}

// Test move semantics
TEST_F(UnorderedCacheTest, MoveSemantics) {
  auto createCache = []() { return UnorderedCache<int, int>(squareCalculator); };

  auto cache = createCache();

  // Populate the cache
  cache.getValue(3);
  cache.getValue(4);

  EXPECT_EQ(cache.getValue(3), 9);
}

// Test with custom hasher
TEST_F(UnorderedCacheTest, CustomHasher) {
  struct CustomIntHasher {
    size_t operator()(const int& key) const {
      return std::hash<int>{}(key) + 42;
    }
  };

  UnorderedCache<int, int, CustomIntHasher> cache(squareCalculator);

  EXPECT_EQ(cache.getValue(5), 25);
  EXPECT_EQ(cache.getValue(5), 25);
}

// Compile-time error tests (these should fail to compile)
#if 0    // These tests are for demonstration and should not actually compile
TEST_F(UnorderedCacheTest, CompileTimeErrorTests) {
    // Test with incompatible calculator return type
    UnorderedCache<int, int> invalidCache1([](const int&) { return "wrong type"; }); // Should not compile

    // Test with incompatible key type
    struct NonHashable {};
    UnorderedCache<NonHashable, int> invalidCache2([](const NonHashable&) { return 42; }); // Should not compile
}
#endif

// Test cache behavior with different key types
TEST_F(UnorderedCacheTest, DifferentKeyTypes) {
  // Test with string keys
  UnorderedCache<std::string, int> stringCache([](const std::string& key) { return key.length(); });

  EXPECT_EQ(stringCache.getValue("hello"), 5);
  EXPECT_EQ(stringCache.getValue("hello"), 5);
}
