
// Filter reads based on the number of minimizers per read (excluding singletons minimzers)
// Author: Amirhossein Afshinfard
// Email: aafshinfard@gmail.com

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <getopt.h>
#include <cstdlib>
#include <chrono>

static std::chrono::time_point<std::chrono::steady_clock> t0;

void printUsage(const std::string& progname) {
    std::cout << "Usage: " << progname
              << " -n n -N N -s singletonFile [-o file] file...\n\n"
                 " -o file         write output to file, default is stdout\n"
                 " -s singletonFile read singleton minimizers from file\n"
                 " -n n            minimum number of minimizers per long read\n"
                 " -N N            maximum number of minimizers per long read\n"
                 " --help          display this help and exit\n"
                 " file            space separated list of files\n";
}

int main(int argc, char* argv[]) {
    t0 = std::chrono::steady_clock::now();
    int c;
    unsigned long n = 0, N = 0;
    std::string outfile = "/dev/stdout";
    std::string singletonFile;
    bool n_set = false, N_set = false, s_set = false;

    while ((c = getopt(argc, argv, "o:n:N:s:")) != -1) {
        switch (c) {
            case 'o':
                outfile.assign(optarg);
                break;
            case 'n':
                n = strtoul(optarg, nullptr, 10);
                n_set = true;
                break;
            case 'N':
                N = strtoul(optarg, nullptr, 10);
                N_set = true;
                break;
            case 's':
                singletonFile.assign(optarg);
                s_set = true;
                break;
            case '?':
                printUsage(argv[0]);
                return 1;
            default:
                abort();
        }
    }

    if (!n_set || !N_set || !s_set) {
        std::cerr << "Options -n, -N, and -s must all be set.\n";
        printUsage(argv[0]);
        return 1;
    }

    // Read singleton minimizers into a set of uint64_t
    std::unordered_set<uint64_t> singletons;
    singletons.reserve(6000000000);
    std::ifstream sfs(singletonFile);
    if (!sfs) {
        std::cerr << "Failed to open singletons file: " << singletonFile << std::endl;
        return 1;
    }
    uint64_t minimizer;
    while (sfs >> minimizer) {
        singletons.insert(minimizer);
    }

    std::ofstream ofs(outfile, std::ofstream::out);
    if (!ofs) {
        std::cerr << "Failed to open output file: " << outfile << std::endl;
        return 1;
    }

    // Process each input file
    for (int index = optind; index < argc; index++) {
        std::string infile = argv[index];
        if (infile == "-") {
            infile = "/dev/stdin";
        }
        std::ifstream ifs(infile);
        if (!ifs) {
            std::cerr << "Failed to open input file: " << infile << std::endl;
            continue; // Proceed to next file
        }

        std::string line;
        while (getline(ifs, line)) {
            std::istringstream iss(line);
            std::string readname;
            iss >> readname; // First part is long-read name
            size_t minimizer_count = 0;
            std::vector<uint64_t> validMinimizers;
            while (iss >> minimizer) {
                if (singletons.find(minimizer) == singletons.end()) {
                    validMinimizers.push_back(minimizer);
                    minimizer_count++;
                }
            }

            if (minimizer_count >= n && minimizer_count < N) {
                ofs << readname;
                for (const auto& m : validMinimizers) {
                    ofs << " " << m;
                }
                ofs << '\n'; // Output the line if it meets criteria
            }
        }
    }

    return 0;
}