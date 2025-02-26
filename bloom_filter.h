#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <vector>
#include <string>
#include <cstdint>
#include <functional>

class BloomFilter {
private:
    std::vector<bool> bits;
    size_t size;
    int hash_functions;
    bool use_fast_hash;
    
    uint32_t fnv1a_hash(const std::string& str) const;
    uint32_t murmur3_hash(const std::string& str, uint32_t seed) const;
    uint32_t fast_hash(const std::string& str, uint32_t seed) const;
    
    uint32_t get_hash_position(const std::string& item, int hash_function_index) const;

public:
    BloomFilter(size_t capacity, double error_rate, bool use_fast_hash = true);
    ~BloomFilter() = default;
    
    void add(const std::string& item);
    bool contains(const std::string& item) const;
    bool contains_batch(const std::vector<std::string>& items, std::vector<bool>& results) const;
};

#endif