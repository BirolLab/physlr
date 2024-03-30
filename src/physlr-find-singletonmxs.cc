// determine singleton minimizers and write them to output

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <getopt.h>
#include <cstdlib>
#include <chrono>

static std::chrono::time_point<std::chrono::steady_clock> t0;

void printUsage(const std::string& progname) {
    std::cout << "Usage: " << progname
              << " [-o file] file...\n\n"
                 " -o file    write output to file, default is stdout\n"
                 " -s         silent; disable verbose output\n"
                 " --help     display this help and exit\n"
                 " file       space separated list of files\n";
}

int main(int argc, char* argv[]) {
    t0 = std::chrono::steady_clock::now();
    bool silent = false;
    int c;
    std::string outfile = "/dev/stdout";

    while ((c = getopt(argc, argv, "o:n:N:s")) != -1) {
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

    std::ofstream ofs(outfile, std::ofstream::out);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << outfile << std::endl;
        return 1;
    }

    // Using a map to track if a minimizer is a singleton (true) or not (false)
    std::unordered_map<uint64_t, bool> singletonMxs;
    singletonMxs.reserve(100000000);

    // Determine singleton status
    auto t1 = std::chrono::steady_clock::now();
    if (!silent) { 
        std::cerr << "Determining singleton status - started at ";
        std::cerr << std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count();
        std::cerr << " (seconds)" << std::endl;
    }
    for (int index = optind; index < argc; ++index) {
        std::string infile = argv[index];
        if (infile == "-") {
            infile = "/dev/stdin";
        }
        std::ifstream ifs(infile);
        if (!ifs) {
            std::cerr << "Failed to open input file: " << infile << std::endl;
            continue; // Proceed to next file
        }

        std::string line, readname;
        uint64_t minimizer;
        while (getline(ifs, line)) {
            std::istringstream iss(line);
            iss >> readname; // First part is long-read name
            while (iss >> minimizer) {
                if (singletonMxs.find(minimizer) == singletonMxs.end()) {
                    // First occurrence
                    singletonMxs[minimizer] = true; // Assume singleton initially
                } else {
                    // Second or more occurrences
                    singletonMxs[minimizer] = false; // Not a singleton
                }
            }
        }
    }
    // now check minimizerSingletonStatus to see if a minimizer is a singleton or not, and write singleton minimizers to output
    t1 = std::chrono::steady_clock::now();
    if (!silent) {
        std::cerr << "Determined singleton minimizers - finished at";
        std::cerr << std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count();
        std::cerr << " (seconds)" << std::endl;
        std::cerr << "Writing singleton minimizers to output" << std::endl;
    }
    for (auto& kv : singletonMxs) {
        if (kv.second) {
            ofs << kv.first << std::endl;
        }
    }
    return 0;
}
