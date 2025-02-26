#include "bloom_filter.h"
#include <cmath>
#include <stdexcept>

BloomFilter::BloomFilter(size_t capacity, double error_rate, bool use_fast_hash) {
    size = static_cast<size_t>(std::ceil(-(capacity * std::log(error_rate)) / (std::log(2) * std::log(2))));
    
    hash_functions = static_cast<int>(std::ceil((size / static_cast<double>(capacity)) * std::log(2)));
    
    if (hash_functions < 1) hash_functions = 1;
    bits.resize(size, false);
    
    this->use_fast_hash = use_fast_hash;
}

uint32_t BloomFilter::fnv1a_hash(const std::string& str) const {
    const uint32_t prime = 16777619;
    uint32_t hash = 2166136261;
    
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= prime;
    }
    
    return hash % size;
}

uint32_t BloomFilter::murmur3_hash(const std::string& str, uint32_t seed) const {
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;
    const uint32_t r1 = 15;
    const uint32_t r2 = 13;
    const uint32_t m = 5;
    const uint32_t n = 0xe6546b64;
    
    uint32_t hash = seed;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(str.c_str());
    const int nblocks = str.length() / 4;
    
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data);
    for (int i = 0; i < nblocks; i++) {
        uint32_t k = blocks[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;
        
        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }
    
    const uint8_t* tail = data + nblocks * 4;
    uint32_t k1 = 0;
    
    switch (str.length() & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: 
            k1 ^= tail[0];
            k1 *= c1;
            k1 = (k1 << r1) | (k1 >> (32 - r1));
            k1 *= c2;
            hash ^= k1;
    }
    
    hash ^= str.length();
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);
    
    return hash % size;
}

uint32_t BloomFilter::fast_hash(const std::string& str, uint32_t seed) const {
    uint32_t hash = seed;
    for (char c : str) {
        hash = hash * 31 + c;
    }
    return hash % size;
}

uint32_t BloomFilter::get_hash_position(const std::string& item, int hash_function_index) const {
    if (use_fast_hash) {
        return fast_hash(item, hash_function_index * 0x9e3779b9);
    } else {
        switch (hash_function_index % 2) {
            case 0: return fnv1a_hash(item);
            case 1: return murmur3_hash(item, hash_function_index * 0x9e3779b9);
            default: return 0;
        }
    }
}

void BloomFilter::add(const std::string& item) {
    for (int i = 0; i < hash_functions; ++i) {
        uint32_t position = get_hash_position(item, i);
        bits[position] = true;
    }
}

bool BloomFilter::contains(const std::string& item) const {
    for (int i = 0; i < hash_functions; ++i) {
        uint32_t position = get_hash_position(item, i);
        if (!bits[position]) {
            return false;
        }
    }
    return true;
}

bool BloomFilter::contains_batch(const std::vector<std::string>& items, std::vector<bool>& results) const {
    results.resize(items.size());
    
    for (size_t i = 0; i < items.size(); i++) {
        results[i] = true;
        for (int j = 0; j < hash_functions; j++) {
            uint32_t position = get_hash_position(items[i], j);
            if (!bits[position]) {
                results[i] = false;
                break;
            }
        }
    }
    
    return true;
} 