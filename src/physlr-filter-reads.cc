// Filter reads based on the number of minimizers per read
// Author: Amirhossein Afshinfard
// Email: aafshinfard@gmail.com

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <getopt.h>
#include <cstdlib>
#include <chrono>
//#include <cstring>

static std::chrono::time_point<std::chrono::steady_clock> t0; // NOLINT(cert-err58-cpp)

// static inline void
// assert_good(const std::ios& stream, const std::string& path)
// {
// 	if (!stream.good()) {
// 		std::cerr << "error: " << strerror(errno) << ": " << path << '\n';
// 		exit(EXIT_FAILURE);
// 	}
// }

void printUsage(const std::string& progname) {
    std::cout << "Usage: " << progname
              << " -n n -N N [-o file] file...\n\n"
                 " -o file    write output to file, default is stdout\n"
                 " -s         silent; disable verbose output\n"
                 " -n         minimum number of minimizers per long read\n"
                 " -N         maximum number of minimizers per long read\n"
                 " --help     display this help and exit\n"
                 " file       space separated list of files\n";
}

int main(int argc, char* argv[]) {
    t0 = std::chrono::steady_clock::now();
    bool silent = false;
    int c;
    unsigned long n = 0, N = 0;
    std::string outfile = "/dev/stdout";
    bool n_set = false, N_set = false;

    while ((c = getopt(argc, argv, "o:n:N:")) != -1) {
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
                silent = true;
                break;
            case '?':
                printUsage(argv[0]);
                return 1;
            default:
                abort();
        }
    }

    if (!n_set || !N_set) {
        std::cerr << "Both -n and -N options must be set.\n";
        printUsage(argv[0]);
        return 1;
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
        //assert_good(ifs, infile);
        if (!ifs) {
            std::cerr << "Failed to open input file: " << infile << std::endl;
            continue; // Proceed to next file
        }

        std::string line;
        while (getline(ifs, line)) {
            std::istringstream iss(line);
            std::string readname, minimizer;
            iss >> readname; // First part is long-read name
            size_t minimizer_count = 0;
            while (iss >> minimizer) {
                minimizer_count++;
            }

            if (minimizer_count >= n && minimizer_count < N) {
                ofs << line << '\n'; // Output the line if it meets criteria
            }
        }
        
        auto t1 = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
        if (!silent) {
            std::cerr << "Time at filter_reads (ms): " << diff.count() << '\n';
            std::cerr << "Wrote " << infile << '\n';
        }
    }

    return 0;
}
