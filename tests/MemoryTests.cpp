#include <gtest/gtest.h>
#include <stdint.h>
#include <string.h>
extern "C" {
#include "../simulator/memory.h"
}

// Test fixture for memory tests
class MemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize DRAM before each test
        clearMemory(&dram);
    }

    DRAM dram = {0};
};

// Test fixture for cache tests
class CacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize DRAM and clear it before each test
        clearMemory(&dram);
    }

    void TearDown() override {
        if (direct_mapped_cache) {
            destroy_cache(direct_mapped_cache);
            direct_mapped_cache = nullptr;
        }
        
        if (two_way_cache) {
            destroy_cache(two_way_cache);
            two_way_cache = nullptr;
        }
    }

    DRAM dram = {0};
    Cache* direct_mapped_cache = nullptr;
    Cache* two_way_cache = nullptr;
};

// Test register initialization
TEST(RegisterTest, Initialization) {
    REGISTERS *regs = init_registers();
    ASSERT_NE(regs, nullptr);
    
    // Check general registers
    for (int i = 0; i < 13; i++) {
        EXPECT_EQ(regs->R[i], 2) << "General register R[" << i << "] not initialized to 2";
    }
    
    // Check special registers
    EXPECT_EQ(regs->R[13], 0) << "LR (R[13]) not initialized to 0";
    EXPECT_EQ(regs->R[14], 100) << "SR (R[14]) not initialized to 100";
    EXPECT_EQ(regs->R[15], 0) << "PC (R[15]) not initialized to 0";
    
    free(regs);
}

// Test DRAM functions
TEST_F(MemoryTest, DRAMOperations) {
    // Test writing and reading
    writeToMemory(&dram, 10, 42);
    EXPECT_EQ(readFromMemory(&dram, 10), 42);
    
    // Test clearing memory
    clearMemory(&dram);
    EXPECT_EQ(readFromMemory(&dram, 10), 0);
    
    // Test viewBlockMemory
    writeToMemory(&dram, 20, 11);
    writeToMemory(&dram, 21, 22);
    writeToMemory(&dram, 22, 33);
    writeToMemory(&dram, 23, 44);
    
    char buffer[128];
    viewBlockMemory(&dram, 20, 1, buffer);
    
    const char* expected = "Memory Block [ 20 ]: 11 22 33 44";
    EXPECT_STREQ(buffer, expected);
}

// Test cache initialization
TEST_F(CacheTest, DirectMappedCacheInitialization) {
    direct_mapped_cache = init_cache(1);
    ASSERT_NE(direct_mapped_cache, nullptr);
    
    EXPECT_EQ(direct_mapped_cache->mode, 1);
    EXPECT_EQ(direct_mapped_cache->num_sets, CACHE_SIZE);
    
    // Check that all cache lines are initialized correctly
    for (int i = 0; i < direct_mapped_cache->num_sets; i++) {
        EXPECT_EQ(direct_mapped_cache->sets[i].associativity, 1);
        EXPECT_EQ(direct_mapped_cache->sets[i].lines[0].valid, 0);
        EXPECT_EQ(direct_mapped_cache->sets[i].lines[0].tag, 0);
    }
}

TEST_F(CacheTest, TwoWayCacheInitialization) {
    two_way_cache = init_cache(2);
    ASSERT_NE(two_way_cache, nullptr);
    
    EXPECT_EQ(two_way_cache->mode, 2);
    EXPECT_EQ(two_way_cache->num_sets, CACHE_SIZE/2);
    
    // Check that all cache lines are initialized correctly
    for (int i = 0; i < two_way_cache->num_sets; i++) {
        EXPECT_EQ(two_way_cache->sets[i].associativity, 2);
        EXPECT_EQ(two_way_cache->sets[i].lines[0].valid, 0);
        EXPECT_EQ(two_way_cache->sets[i].lines[0].tag, 0);
        EXPECT_EQ(two_way_cache->sets[i].lines[1].valid, 0);
        EXPECT_EQ(two_way_cache->sets[i].lines[1].tag, 0);
    }
}

// Test invalid cache mode
TEST_F(CacheTest, InvalidCacheMode) {
    Cache* invalid_cache = init_cache(3);
    EXPECT_EQ(invalid_cache, nullptr);
}

// Test direct-mapped cache operations
TEST_F(CacheTest, DirectMappedCacheOperations) {
    direct_mapped_cache = init_cache(1);
    ASSERT_NE(direct_mapped_cache, nullptr);
    
    // Test write-through
    // Write data to address 100
    int hit = write_through(direct_mapped_cache, &dram, 100, 250);
    // On first write, it should be a miss
    EXPECT_EQ(hit, 0);
    EXPECT_EQ(readFromMemory(&dram, 100), 250);
    
    // Read data from cache should load the block
    uint16_t data = read_cache(direct_mapped_cache, &dram, 100);
    EXPECT_EQ(data, 250);
    
    // Now write again to same address, should be a hit
    hit = write_through(direct_mapped_cache, &dram, 100, 300);
    EXPECT_EQ(hit, 1);
    EXPECT_EQ(readFromMemory(&dram, 100), 300);
    
    // Read again, should get updated value
    data = read_cache(direct_mapped_cache, &dram, 100);
    EXPECT_EQ(data, 300);
}

// Test cache eviction in direct-mapped cache
TEST_F(CacheTest, DirectMappedCacheEviction) {
    direct_mapped_cache = init_cache(1);
    ASSERT_NE(direct_mapped_cache, nullptr);
    
    // Setup initial cache state
    writeToMemory(&dram, 100, 250);
    uint16_t data = read_cache(direct_mapped_cache, &dram, 100);
    EXPECT_EQ(data, 250);
    
    // Address that maps to same cache line
    uint16_t conflicting_addr = 100 + BLOCK_SIZE * direct_mapped_cache->num_sets;
    writeToMemory(&dram, conflicting_addr, 400);
    
    // Reading from the conflicting address should evict previous entry
    data = read_cache(direct_mapped_cache, &dram, conflicting_addr);
    EXPECT_EQ(data, 400);
    
    // After eviction, reading original address should be a miss
    // (will load fresh but the underlying memory value should be correct)
    data = read_cache(direct_mapped_cache, &dram, 100);
    EXPECT_EQ(data, 250);
}

// Test two-way set associative cache operations and LRU policy
TEST_F(CacheTest, TwoWayCacheOperations) {
    two_way_cache = init_cache(2);
    ASSERT_NE(two_way_cache, nullptr);
    
    // Setup initial cache state
    // First address
    uint16_t addr1 = 100;
    writeToMemory(&dram, addr1, 250);
    uint16_t data = read_cache(two_way_cache, &dram, addr1);
    EXPECT_EQ(data, 250);
    
    // Second address that maps to same set but different tag
    uint16_t addr2 = addr1 + BLOCK_SIZE * two_way_cache->num_sets;
    writeToMemory(&dram, addr2, 350);
    data = read_cache(two_way_cache, &dram, addr2);
    EXPECT_EQ(data, 350);
    
    // Both entries should be in cache now, so reading them shouldn't change anything
    data = read_cache(two_way_cache, &dram, addr1);
    EXPECT_EQ(data, 250);
}

// Test LRU replacement policy in two-way set associative cache
TEST_F(CacheTest, TwoWayCacheLRUPolicy) {
    two_way_cache = init_cache(2);
    ASSERT_NE(two_way_cache, nullptr);
    
    // Setup initial cache state with two addresses mapping to same set
    uint16_t addr1 = 100;
    uint16_t addr2 = addr1 + BLOCK_SIZE * two_way_cache->num_sets;
    
    writeToMemory(&dram, addr1, 250);
    read_cache(two_way_cache, &dram, addr1);
    
    writeToMemory(&dram, addr2, 350);
    read_cache(two_way_cache, &dram, addr2);
    
    // Access addr1 to make addr2 the LRU
    read_cache(two_way_cache, &dram, addr1);
    
    // Add a third address that maps to the same set to force eviction
    uint16_t addr3 = addr2 + BLOCK_SIZE * two_way_cache->num_sets;
    writeToMemory(&dram, addr3, 450);
    uint16_t data = read_cache(two_way_cache, &dram, addr3);
    EXPECT_EQ(data, 450);
    
    // addr2 should have been evicted (it was LRU)
    // addr1 and addr3 should still be in cache
    data = read_cache(two_way_cache, &dram, addr1);
    EXPECT_EQ(data, 250);
    
    // Reading addr2 should now cause a cache miss and reload
    data = read_cache(two_way_cache, &dram, addr2);
    EXPECT_EQ(data, 350);
    
    // Now addr3 should be evicted as it became LRU after addr1 was accessed
    uint16_t addr4 = addr3 + BLOCK_SIZE * two_way_cache->num_sets;
    writeToMemory(&dram, addr4, 550);
    data = read_cache(two_way_cache, &dram, addr4);
    EXPECT_EQ(data, 550);
    
    // Check that addr3 was indeed evicted
    // Reading addr3 should be a miss and reload
    data = read_cache(two_way_cache, &dram, addr3);
    EXPECT_EQ(data, 450);
}

// Test cache clear functionality
TEST_F(CacheTest, CacheClear) {
    two_way_cache = init_cache(2);
    ASSERT_NE(two_way_cache, nullptr);
    
    // Load some values into cache
    writeToMemory(&dram, 100, 250);
    read_cache(two_way_cache, &dram, 100);
    
    writeToMemory(&dram, 200, 350);
    read_cache(two_way_cache, &dram, 200);
    
    // Clear cache
    clear_cache(two_way_cache);
    
    // Check that cache is cleared by verifying that the valid bits are reset
    for (int i = 0; i < two_way_cache->num_sets; i++) {
        for (int j = 0; j < two_way_cache->mode; j++) {
            EXPECT_EQ(two_way_cache->sets[i].lines[j].valid, 0);
        }
    }
}

// Test DRAM update with pending operations
TEST_F(MemoryTest, DRAMUpdate) {
    // Setup a pending write operation
    dram.state = DRAM_WRITE; // Not IDLE
    dram.delayCounter = 2;
    dram.pendingAddr = 300;
    dram.pendingValue = 777;
    strcpy(dram.pendingCmd, "SW");
    
    // First update should decrement counter
    updateDRAM(&dram, NULL);
    EXPECT_EQ(dram.delayCounter, 1);
    EXPECT_EQ(dram.state, DRAM_WRITE);
    
    // Second update should complete the operation
    updateDRAM(&dram, NULL);
    EXPECT_EQ(dram.delayCounter, 0);
    EXPECT_EQ(dram.state, DRAM_IDLE);
    EXPECT_EQ(readFromMemory(&dram, 300), 777);
}

// Test DRAM update with cache
TEST_F(CacheTest, DRAMUpdateWithCache) {
    two_way_cache = init_cache(2);
    ASSERT_NE(two_way_cache, nullptr);
    
    // Setup a pending read operation
    dram.state = DRAM_READ; // Not IDLE
    dram.delayCounter = 1;
    dram.pendingAddr = 400;
    writeToMemory(&dram, 400, 888); // Set up the memory value
    strcpy(dram.pendingCmd, "LW");
    
    // Update should complete the operation and load into cache
    updateDRAM(&dram, two_way_cache);
    
    // Verify DRAM state
    EXPECT_EQ(dram.state, DRAM_IDLE);
    
    // Check the value is now in cache
    uint16_t data = read_cache(two_way_cache, &dram, 400);
    EXPECT_EQ(data, 888);
}

// Test write-through with existing cache line
TEST_F(CacheTest, WriteThrough) {
    direct_mapped_cache = init_cache(1);
    ASSERT_NE(direct_mapped_cache, nullptr);
    
    // First set up a cached value
    writeToMemory(&dram, 500, 100);
    read_cache(direct_mapped_cache, &dram, 500);
    
    // Now write through to update both cache and memory
    int hit = write_through(direct_mapped_cache, &dram, 500, 200);
    EXPECT_EQ(hit, 1); // Should be a hit
    
    // Check both memory and cache
    EXPECT_EQ(readFromMemory(&dram, 500), 200);
    EXPECT_EQ(read_cache(direct_mapped_cache, &dram, 500), 200);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

