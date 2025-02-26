#ifndef BITCOIN_UTILS_H
#define BITCOIN_UTILS_H

#include <string>
#include <vector>
#include <gmpxx.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <secp256k1.h>
#include <unordered_map>
#include <mutex>

extern std::unordered_map<std::string, std::string> address_cache;
extern std::mutex cache_mutex;

std::string bytes_to_hex(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> hex_to_bytes(const std::string& hex);
std::string base58_encode(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
std::vector<uint8_t> ripemd160(const std::vector<uint8_t>& data);
std::vector<uint8_t> private_key_to_compressed_public_key(const std::string& private_key_hex);
std::string private_key_to_compressed_address(const std::string& private_key_hex);

#endif