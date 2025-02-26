cmake_minimum_required(VERSION 3.10)
project(BeenScan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_library(GMP_LIBRARIES gmp)
find_library(GMPXX_LIBRARIES gmpxx)
find_library(SECP256K1_LIBRARIES secp256k1)

if(NOT GMP_LIBRARIES)
    message(FATAL_ERROR "GMP library not found")
endif()

if(NOT GMPXX_LIBRARIES)
    message(FATAL_ERROR "GMPXX library not found")
endif()

if(NOT SECP256K1_LIBRARIES)
    message(FATAL_ERROR "secp256k1 library not found")
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <cpuid.h>
    int main() {
        unsigned int eax, ebx, ecx, edx;
        __get_cpuid(1, &eax, &ebx, &ecx, &edx);
        return (ecx & bit_AES) ? 0 : 1;
    }
" HAVE_AES_NI)

if(HAVE_AES_NI)
    add_definitions(-DHAVE_AES_NI)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(been_scan
    main.cpp
    bitcoin_utils.cpp
    bloom_filter.cpp
)

target_include_directories(been_scan PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(been_scan
    ${OPENSSL_LIBRARIES}
    ${GMP_LIBRARIES}
    ${GMPXX_LIBRARIES}
    ${SECP256K1_LIBRARIES}
)