BEEN SCAN - Random+Sequential Bitcoin Private Key Searcher
=======================================

A high-performance Bitcoin private key scanner that searches for specific Bitcoin addresses within a given range of private keys.

Features:
- Multi-threaded processing for maximum performance
- Bloom filter for efficient address checking
- Real-time statistics on scanning speed
- Caching of generated addresses for improved performance

Installation (Linux)
-------------------

1. Install required dependencies:

   ```
   sudo apt update
   sudo apt install -y build-essential cmake git libssl-dev libgmp-dev
   ```

2. Install secp256k1 library:

   ```
   git clone https://github.com/bitcoin-core/secp256k1.git
   cd secp256k1
   ./autogen.sh
   ./configure --enable-module-recovery
   make
   sudo make install
   sudo ldconfig
   ```

   NOTE: If you need to install the autogen prerequisites:
   ```
   sudo apt install -y autoconf automake libtool pkg-config
   ```

3. Build Been Scan:

   ```
   cd /path/to/been_scan
   mkdir -p build
   cd build
   cmake ..
   make
   ```

Usage
-----

1. Create a file named "address.txt" in the same directory as the executable, containing Bitcoin addresses you want to search for (one per line).

2. Run the program:

   ```
   ./been_scan
   ```

3. Enter the hexadecimal range for searching:
   - Start private key (e.g., 0000000000000000000000000000000000000000000000000000000000000001)
   - End private key (e.g., FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140)

4. The program will display real-time statistics during the search.

5. If matching addresses are found, they will be saved to "results.txt" along with the corresponding private keys.

Performance Notes
----------------

- The program automatically determines the optimal number of threads for your CPU
- Use CPU with AES-NI support for better performance
- Performance depends on your hardware: a modern CPU can check millions of keys per second
- The larger your set of target addresses, the more memory will be required

Troubleshooting
--------------

If you encounter "library not found" errors when running the program:
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

If you want to check if secp256k1 is installed correctly:
```
sudo find / -name libsecp256k1.so
```

Bitcoin Tips Address
-------------------
If you find this software useful, consider sending a tip to:
198DXhcUTDUdBfNZRJzrWdS4LFuif5eb2L
