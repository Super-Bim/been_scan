#include "bitcoin_utils.h"
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <mutex>

thread_local std::vector<uint8_t> tl_buffer(100);

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : bytes) {
        ss << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

static const char* BASE58_CHARS = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string base58_encode(const std::vector<uint8_t>& bytes) {
    mpz_class number = 0;
    
    for (uint8_t byte : bytes) {
        number = number * 256 + byte;
    }
    
    std::string result;
    while (number > 0) {
        mpz_class remainder = number % 58;
        result = BASE58_CHARS[remainder.get_ui()] + result;
        number /= 58;
    }
    
    for (size_t i = 0; i < bytes.size() && bytes[i] == 0; i++) {
        result = '1' + result;
    }
    
    return result;
}

std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
    tl_buffer.resize(SHA256_DIGEST_LENGTH);
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(tl_buffer.data(), &sha256);
    
    return tl_buffer;
}

std::vector<uint8_t> ripemd160(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(RIPEMD160_DIGEST_LENGTH);
    
    RIPEMD160_CTX ripemd160;
    RIPEMD160_Init(&ripemd160);
    RIPEMD160_Update(&ripemd160, data.data(), data.size());
    RIPEMD160_Final(hash.data(), &ripemd160);
    
    return hash;
}

std::vector<uint8_t> private_key_to_compressed_public_key(const std::string& private_key_hex) {
    std::vector<uint8_t> privkey = hex_to_bytes(private_key_hex);
    std::vector<uint8_t> pubkey(33);
    
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    
    secp256k1_pubkey pubkey_secp;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey_secp, privkey.data())) {
        secp256k1_context_destroy(ctx);
        throw std::runtime_error("Failed to create public key");
    }
    
    size_t output_len = pubkey.size();
    if (!secp256k1_ec_pubkey_serialize(ctx, pubkey.data(), &output_len, &pubkey_secp, 
                                      SECP256K1_EC_COMPRESSED)) {
        secp256k1_context_destroy(ctx);
        throw std::runtime_error("Failed to serialize public key");
    }
    
    secp256k1_context_destroy(ctx);
    return pubkey;
}

std::unordered_map<std::string, std::string> address_cache;
std::mutex cache_mutex;

std::string private_key_to_compressed_address(const std::string& private_key_hex) {
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = address_cache.find(private_key_hex);
        if (it != address_cache.end()) {
            return it->second;
        }
    }
    
    std::vector<uint8_t> public_key = private_key_to_compressed_public_key(private_key_hex);
    
    std::vector<uint8_t> sha256_result = sha256(public_key);
    
    std::vector<uint8_t> ripemd160_result = ripemd160(sha256_result);
    
    std::vector<uint8_t> network_byte = {0x00};
    network_byte.insert(network_byte.end(), ripemd160_result.begin(), ripemd160_result.end());
    
    std::vector<uint8_t> first_sha = sha256(network_byte);
    std::vector<uint8_t> second_sha = sha256(first_sha);
    
    std::vector<uint8_t> address_bytes = network_byte;
    address_bytes.insert(address_bytes.end(), second_sha.begin(), second_sha.begin() + 4);
    
    std::string result = base58_encode(address_bytes);
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        if (address_cache.size() > 10000) {
            address_cache.clear();
        }
        address_cache[private_key_hex] = result;
    }
    
    return result;
}