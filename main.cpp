#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <random>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include "bitcoin_utils.h"
#include "bloom_filter.h"

const uint64_t MAX_PRIVATE_KEY_1 = 0xFFFFFFFFFFFFFFFF;
const uint64_t MAX_PRIVATE_KEY_2 = 0xFFFFFFFFFFFFFFFE;
const uint64_t MAX_PRIVATE_KEY_3 = 0xBAAEDCE6AF48A03B;
const uint64_t MAX_PRIVATE_KEY_4 = 0xBFD25E8CD0364140;
const int KEYS_AMOUNT = 500000;
const std::string ADDRESS_FILE = "address.txt";
const std::string OUTPUT_FILE = "results.txt";

std::mutex mtx_console;
std::mutex mtx_result;
std::atomic<uint64_t> global_keys_checked(0);
std::atomic<bool> key_found(false);

std::pair<BloomFilter*, std::set<std::string>> load_addresses() {
    try {
        std::ifstream file(ADDRESS_FILE);
        if (!file.is_open()) {
            std::cerr << "Error: File " << ADDRESS_FILE << " not found." << std::endl;
            return {nullptr, std::set<std::string>()};
        }

        std::set<std::string> target_addresses;
        BloomFilter* bloom_filter = new BloomFilter(1000000, 0.001);
        
        std::string address;
        while (std::getline(file, address)) {
            if (!address.empty()) {
                address.erase(address.find_last_not_of("\r\n") + 1);
                target_addresses.insert(address);
                bloom_filter->add(address);
            }
        }
        
        file.close();
        return {bloom_filter, target_addresses};
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading addresses: " << e.what() << std::endl;
        return {nullptr, std::set<std::string>()};
    }
}

void save_results(const std::vector<std::pair<std::string, std::string>>& results) {
    std::ofstream file(OUTPUT_FILE, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Error opening file to save results." << std::endl;
        return;
    }

    for (const auto& [private_key, address] : results) {
        file << "Private Key: " << private_key << std::endl;
        file << "Address: " << address << std::endl;
        file << "------------------------------" << std::endl;
    }
    
    file.close();
}

std::string int_to_hex(uint64_t value, int width) {
    std::stringstream ss;
    ss << std::hex << std::setw(width) << std::setfill('0') << value;
    return ss.str();
}

void check_interval(const mpz_class& start_key, const mpz_class& end_key, 
                     BloomFilter* bloom_filter, const std::set<std::string>& target_addresses) {
    for (mpz_class index = start_key; index <= end_key; index++) {
        global_keys_checked++;
        
        std::string private_key = index.get_str(16);
        while (private_key.length() < 64) {
            private_key = "0" + private_key;
        }
        
        std::string compressed_address = private_key_to_compressed_address(private_key);

        if (bloom_filter->contains(compressed_address)) {
            if (target_addresses.find(compressed_address) != target_addresses.end()) {
                std::lock_guard<std::mutex> lock(mtx_console);
                std::cout << "\nFound! Key: " << private_key 
                          << " -> Address: " << compressed_address << std::endl;

                std::lock_guard<std::mutex> lock_result(mtx_result);
                std::vector<std::pair<std::string, std::string>> result = {
                    {private_key, compressed_address}
                };
                save_results(result);
                
                key_found = true;
                break;
            }
        }
        
        if (key_found) break;
    }
}

void search_random_range(const mpz_class& start_range, 
                         const mpz_class& end_range, 
                         int keys_amount) {
    auto [bloom_filter, target_addresses] = load_addresses();
    if (bloom_filter == nullptr) {
        std::cout << "Could not load target addresses. Exiting..." << std::endl;
        return;
    }

    std::cout << "Defined range: from 0x" << start_range.get_str(16) 
              << " to 0x" << end_range.get_str(16) << std::endl;

    unsigned int available_threads = std::thread::hardware_concurrency();
    std::cout << "Your system has " << available_threads << " available threads." << std::endl;
    
    unsigned int num_threads;
    std::cout << "Enter the number of threads to use (recommended: 1-" << available_threads << "): ";
    std::cin >> num_threads;
    
    if (num_threads < 1) {
        std::cout << "Invalid number of threads. Using 1 thread." << std::endl;
        num_threads = 1;
    } else if (num_threads > 64) {
        std::cout << "Warning: Using a very high number of threads may affect system performance." << std::endl;
        std::cout << "Do you want to continue with " << num_threads << " threads? (y/n): ";
        char confirm;
        std::cin >> confirm;
        if (confirm != 'y' && confirm != 'Y') {
            num_threads = available_threads;
            std::cout << "Using " << num_threads << " threads instead." << std::endl;
        }
    }

    std::cout << "Using " << num_threads << " threads for parallel processing." << std::endl;

    std::vector<std::thread> threads;
    mpz_class keys_per_thread = (end_range - start_range + 1) / num_threads;
    
    std::cout << "Total range will be divided into " << num_threads << " segments." << std::endl;
    
    for (unsigned int i = 0; i < num_threads; i++) {
        mpz_class thread_start = start_range + i * keys_per_thread;
        mpz_class thread_end = (i == num_threads - 1) ? end_range : thread_start + keys_per_thread - 1;
        
        mpz_class num_keys = thread_end - thread_start + 1;
        
        std::cout << "Thread " << i+1 << " will check range: from 0x" << thread_start.get_str(16)
                  << " to 0x" << thread_end.get_str(16) 
                  << " (" << num_keys.get_str(10) << " keys)" << std::endl;
        
        threads.push_back(std::thread(check_interval, thread_start, thread_end, 
                                      bloom_filter, std::ref(target_addresses)));
    }
    
    std::atomic<bool> running(true);
    std::thread stats_thread([&]() {
        auto start_time = std::chrono::high_resolution_clock::now();
        uint64_t last_count = 0;
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            auto now = std::chrono::high_resolution_clock::now();
            auto total_time = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            
            uint64_t current_count = global_keys_checked;
            uint64_t keys_last_second = current_count - last_count;
            double total_rate = total_time > 0 ? static_cast<double>(current_count) / total_time : 0;
            
            std::lock_guard<std::mutex> lock(mtx_console);
            std::cout << "\rChecked: " << current_count 
                      << " | Last second: " << keys_last_second 
                      << " | Average: " << std::fixed << std::setprecision(2) << total_rate 
                      << " keys/sec | Time: " << total_time << "s" << std::flush;
            
            last_count = current_count;
        }
    });
    
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    running = false;
    if (stats_thread.joinable()) stats_thread.join();
    
    delete bloom_filter;
}

int main() {
    std::cout << "Search for Bitcoin private keys in a range" << std::endl;
    std::string start_hex, end_hex;
    
    std::cout << "Enter the start private key of the range (in hexadecimal): ";
    std::cin >> start_hex;
    
    std::cout << "Enter the end private key of the range (in hexadecimal): ";
    std::cin >> end_hex;

    try {
        mpz_class start_range(start_hex, 16);
        mpz_class end_range(end_hex, 16);
        
        mpz_class MAX_PRIVATE_KEY("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140", 16);
        
        if (start_range < 0 || 
            end_range > MAX_PRIVATE_KEY || 
            start_range >= end_range) {
            std::cout << "Invalid range. Make sure the start key is less than the end key and both are within the allowed limit." << std::endl;
            return 1;
        }

        search_random_range(start_range, end_range, KEYS_AMOUNT);
    }
    catch (const std::exception& e) {
        std::cout << "Error: Invalid inputs. Both keys must be in hexadecimal format." << std::endl;
        std::cerr << "Error details: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
