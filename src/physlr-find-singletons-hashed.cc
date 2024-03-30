#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <getopt.h>
#include <cstdlib>
#include <chrono>
#include <functional>

static std::chrono::time_point<std::chrono::steady_clock> t0;

void printUsage(const std::string& progname) {
    std::cout << "Usage: " << progname
              << " [-o file] file...\n\n"
                 " -o file    write output to file, default is stdout\n"
                 " -s         silent; disable verbose output\n"
                 " --help     display this help and exit\n"
                 " file       space separated list of files\n";
}

// A simple hash function to reduce uint64_t to a smaller range.
size_t hashMinimizer(uint64_t minimizer, size_t hashSize) {
    std::hash<uint64_t> hashFn;
    return hashFn(minimizer) % hashSize;
}

int main(int argc, char* argv[]) {
    t0 = std::chrono::steady_clock::now();
    bool silent = false;
    std::string outfile = "/dev/stdout";
    size_t hashSize = 200000000000; // Size of the hash tables
    // size_t hashSize = 170000000000; // 40 GB (2x20 GB)
    int c;
    while ((c = getopt(argc, argv, "o:s")) != -1) {
        switch (c) {
            case 'o':
                outfile.assign(optarg);
                break;
            case 's':
                silent = true;
                break;
            case '?':
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    std::vector<bool> seenOnce(hashSize, false);
    std::vector<bool> seenMoreThanOnce(hashSize, false);

    std::ofstream ofs(outfile, std::ofstream::out);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << outfile << std::endl;
        return 1;
    }

    for (int index = optind; index < argc; ++index) {
        std::string infile = argv[index];
        std::ifstream ifs(infile);
        if (!ifs) {
            std::cerr << "Failed to open input file: " << infile << std::endl;
            continue;
        }

        std::string line, readname;
        uint64_t minimizer;
        while (getline(ifs, line)) {
            std::istringstream iss(line);
            iss >> readname;
            while (iss >> minimizer) {
                size_t hashedIndex = hashMinimizer(minimizer, hashSize);
                if (!seenOnce[hashedIndex]) {
                    seenOnce[hashedIndex] = true;
                } else {
                    seenMoreThanOnce[hashedIndex] = true;
                }
            }
        }
    }

    // Final output: Write hashed indices of singleton minimizers to output file
    for (size_t i = 0; i < hashSize; ++i) {
        // If seen once but not more than once, it's considered a singleton
        if (seenOnce[i] && !seenMoreThanOnce[i]) {
            ofs << i << std::endl; // Output the hashed index
        }
    }

    if (!silent) {
        auto t1 = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count();
        std::cerr << "Process completed in " << duration << " seconds." << std::endl;
    }

        
        return 0;
}
