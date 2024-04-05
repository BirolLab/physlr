#include "btllib/bloom_filter.hpp"
#include "btllib/util.hpp"
#include <argparse/argparse.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
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
    * IMPROVE: 
    * 1. Before running the program, use a command line command to remove reads with few minimizers -> fewer reads to process and fewer minimizers to keep track of. 
    * 2. Add a Bloom filter to keep track of new singletons after removing reads with few minimizers, instead of runnning another code of two passes -> saves one pass.
    * 3. Parallelize the program.
    * 
*/

void populateBFs(const std::string& filePath, btllib::KmerBloomFilter* firstOcc, btllib::KmerBloomFilter* secondOcc) {
    std::ifstream inFile(filePath);
    std::string line, readName;
    uint64_t minimizer;
    // count lines and print a log after every 500000 lines
    unsigned int lineCount = 0;
    while (getline(inFile, line)) {
        ++lineCount;
        std::istringstream iss(line);
        iss >> readName; // Skip read name

        while (iss >> minimizer) {
            const uint64_t* minimizerPtr = &minimizer;
            if (!firstOcc->contains(minimizerPtr)) {
                firstOcc->insert(minimizerPtr);
            } else if (!secondOcc->contains(minimizerPtr)) {
                secondOcc->insert(minimizerPtr);
            }
        }
        if (lineCount % 500000 == 0) {
            std::cerr << "Processed " << lineCount << " reads to populate cascading Bloom filters" << std::endl;
        }
    }
}

void filterReads(const std::string& filePath, btllib::KmerBloomFilter* secondOcc, const std::string& outFilePath, unsigned int min_mxs, unsigned int max_mxs) {
    std::ifstream inFile(filePath);
    std::ofstream outFile(outFilePath);
    std::string line, readName;
    uint64_t minimizer;
    unsigned int lineCount = 0;
    while (getline(inFile, line)) {
        ++lineCount;
        std::istringstream iss(line);
        iss >> readName;
        std::vector<uint64_t> minimizers;
        while (iss >> minimizer) {
            const uint64_t* minimizerPtr = &minimizer;
            if (secondOcc->contains(minimizerPtr)) {
                minimizers.push_back(minimizer);
            }
        }

        if (minimizers.size() >= min_mxs && minimizers.size() <= max_mxs) {
            outFile << readName;
            for (const auto& min : minimizers) {
                outFile << " " << min;
            }
            outFile << std::endl;
        }
        if (lineCount % 500000 == 0) {
            std::cerr << "Processed " << lineCount << " reads to filter reads based on minimizer count" << std::endl;
        }
    }
}

int main(int argc, const char* argv[]) {
    argparse::ArgumentParser program("filter-readmxs");

    program.add_argument("-i", "--input")
           .help("Input file containing minimizers")
           .required();

    program.add_argument("-o", "--output")
           .help("Output file for filtered reads")
           .required();

    program.add_argument("--fpr")
           .help("False positive rate for Bloom filter")
           .default_value(0.025)
           .scan<'g', double>();

    program.add_argument("--bf")
           .help("Size of the Bloom filter")
           .default_value(10000000)
           .scan<'u', unsigned int>();
    
    program.add_argument("-k")
           .help("k-mer size (bp)")
           .required()
           .scan<'u', unsigned>();
    
    program.add_argument("--min")
           .help("Minimum number of minimizers per read to keep")
           .default_value(1)
           .scan<'u', unsigned int>();

    program.add_argument("--max")
           .help("Maximum number of minimizers per read to keep")
           .default_value(100000)
           .scan<'u', unsigned int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    unsigned int bloomSize = program.get<unsigned int>("bf");
    double fpr = program.get<double>("fpr");
    std::string inputFilePath = program.get<std::string>("input");
    std::string outputFilePath = program.get<std::string>("output");
    unsigned int min_min = program.get<unsigned int>("min");
    unsigned int max_min = program.get<unsigned int>("max");
    unsigned k = program.get<unsigned>("k");

    // print the parameters
    std::cerr << "\t\t--input: " << inputFilePath << std::endl;
    std::cerr << "\t\t--output: " << outputFilePath << std::endl;
    std::cerr << "\t\t--fpr: " << fpr << std::endl;
    std::cerr << "\t\t--bf: " << bloomSize << std::endl;
    std::cerr << "\t\t-k: " << k << std::endl;
    std::cerr << "\t\t--min: " << min_min << std::endl;
    std::cerr << "\t\t--max: " << max_min << std::endl;
    
    btllib::KmerBloomFilter* firstOcc = 
    new btllib::KmerBloomFilter(bloomSize, 1, k);
    btllib::KmerBloomFilter* secondOcc =
    new btllib::KmerBloomFilter(bloomSize, 1, k);
    

    populateBFs(inputFilePath, firstOcc, secondOcc);
    delete firstOcc;
    // improve: add another bloom filter to keep track of new singletons after removing reads with few minimizers
    filterReads(inputFilePath, secondOcc, outputFilePath, min_min, max_min);

    return 0;
}