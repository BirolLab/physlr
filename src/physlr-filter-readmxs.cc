#include "btllib/bloom_filter.hpp"
//#include "btllib/seq_reader.hpp"
#include "btllib/util.hpp"
#include <argparse/argparse.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#if _OPENMP
#include <omp.h>
#endif

/*
    * This program reads output of indexlr, loads a cascading bloom filter (2xbinary), removes singelton minimizers, and filters reads with few or numerous minimizers.
    * The input file should contain a read-name and then a list of minimizers in each line.
    * The program uses a Bloom filter to identify unique minimizers.
    * The program outputs in the same format as the input file, excluding the singletons and reads with few or numerous minimizers.
    *
    * Author: Amirhossein Afshinfard <
    * email: aafshinfard@bcgsc.ca
*/

void populateBFs(const std::string& filePath, btllib::KmerBloomFilter* firstOcc, btllib::KmerBloomFilter* secondOcc) {
    std::ifstream inFile(filePath);
    std::string line, readName, minimizer;
    while (getline(inFile, line)) {
        std::istringstream iss(line);
        iss >> readName; // Skip read name

        while (iss >> minimizer) {
            // the minimizer is already a hash value so use it directly
            if (!firstOcc->contains(minimizer)) {
                firstOcc->insert(minimizer);
            } else if (!secondOcc->contains(minimizer)) {
                secondOcc->insert(minimizer);
            }
        }
    }
}

void filterReads(const std::string& filePath, btllib::KmerBloomFilter* firstOcc, btllib::KmerBloomFilter* secondOcc, bool keepSingletons=false, const std::string& outFilePath, unsigned int min_min, unsigned int max_min); {
    std::ifstream inFile(filePath);
    std::ofstream outFile(outFilePath);
    std::string line, readName;
    uint64_t minimizer;
    while (getline(inFile, line)) {
        std::istringstream iss(line);
        iss >> readName;
        std::vector<uint64_t> minimizers;
        while (iss >> minimizer) {
            // only push minimizer if its in the second bloom filter
            if (secondOcc->contains(minimizer)) {
                minimizers.push_back(minimizer);
            }
        }

        if (minimizers.size() < min_min || minimizers.size() > max_min) {
            continue;
        }

        outFile << readName;
        for (const auto& minimizer : minimizers) {
            outFile << " " << minimizer;
        }
        outFile << std::endl;
    }
}

int main(int argc, const char* argv[]) {
    
    argparse::ArgumentParser program("filter-readmxs");
    
    program.add_argument("-i", "--input")
    .help("Input file containing minimizers")
    .required();

    program.add_argument("-o", "--output")
    .help("Output file containing filtered minimizers")
    .required();

    program.add_argument("-t")
    .help("Number of threads")
    .default_value((unsigned int)1)
    .scan<'u', unsigned int>();

    parser.add_argument("--fpr")
    .help("False positive rate for Bloom filter")
    .default_value((double)0.025)
    .scan<'g', double>();
    
    parser.add_argument("--bf")
    .help("Size of the Bloom filter")
    .default_value((unsigned int)10000000)
    .scan<'u', unsigned int>();
    
    parser.add_argument("--keep-singletons")
    .help("Keep singleton minimizers")
    .default_value(false)
    .implicit_value(true);
    
    parser.add_argument("--max")
    .help("Maximum number of minimizers per read")
    .default_value((unsigned int)1000)
    .scan<'u', unsigned int>();

    parser.add_argument("--min")
    .help("Minimum number of minimizers per read")
    .default_value((unsigned int)1)
    .scan<'u', unsigned int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }

    unsigned num_threads = parser.get<unsigned>("t");
    double fpr = parser.get<double>("fpr");
    unsigned bloomSize = parser.get<unsigned>("bf");
    bool keepSingletons = parser.get<bool>("keep-singletons");
    unsigned int min_min = parser.get<unsigned>("min");
    unsigned int max_min = parser.get<unsigned>("max");
    std::string inputFilePath = parser.get<std::string>("input");
    std::string outputFilePath = parser.get<std::string>("output");
    
    // print the arguments
    std::cout << "\t\t-t: " << num_threads << std::endl;
    std::cout << "\t\t--fpr: " << fpr << std::endl;
    std::cout << "\t\t--bf: " << bloomSize << std::endl;
    std::cout << "\t\t--keep-singletons: " << keepSingletons << std::endl;
    std::cout << "\t\t--min: " << min_min << std::endl;
    std::cout << "\t\t--max: " << max_min << std::endl;
    std::cout << "\t\t--input: " << inputFilePath << std::endl;
    std::cout << "\t\t--output: " << outputFilePath << std::endl;
    
    btllib::KmerBloomFilter firstOcc(bloomSize, fpr);
    btllib::KmerBloomFilter secondOcc(bloomSize, fpr);

    populateBFs(inputFilePath, &firstOcc, &secondOcc);
    filterReads(inputFilePath, &firstOcc, &secondOcc, outputFilePath, keepSingletons, min_min, max_min);

    return 0;
}