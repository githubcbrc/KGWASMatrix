#include "mmap_io.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>

// for mmap:
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

//for converting [const char*] to [istream]
#include <istream>
#include <streambuf>
#include <string>



struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};


void handle_error(std::string msg) {
    perror(msg.c_str()); 
    exit(-1); //TODO: break and go to listening mode
}


void get_reads(std::string fname, std::vector<std::pair<std::string, std::string>>& test_set, size_t max_size)
{
    int fd = open(fname.c_str(), O_RDONLY);
    if (fd == -1)
        handle_error("open");

    struct stat sb;
    if (fstat(fd, &sb) == -1)
        handle_error("fstat");

    size_t size_ = sb.st_size;

    char* f = static_cast<char*>(mmap(NULL, size_, PROT_READ, MAP_PRIVATE, fd, 0u));
    if (f == MAP_FAILED)
        handle_error("mmap");

    auto l = f + size_;
    membuf sbuf(f, l);
    std::istream in(&sbuf);
    std::string line;

    std::vector<std::string> seqs;
    std::vector<std::string> ids;
    test_set.reserve((int)1e6);
    seqs.reserve((int)1e6);
    ids.reserve((int)1e6);

    std::string format;
    getline(in, line);
    if (line[0] == '>') format = "fasta";
    else if (line[0] == '@') format = "fastq";
    else handle_error("unrecognized file format");
    ids.push_back(line.substr(1,line.find(" ")));
    size_t i = 1;

    if (format == "fastq") {
        std::cout << "handling fastq input ... " << std::endl;
        while (getline(in, line)) {
            if (i%4==0) ids.push_back(line.substr(1,line.find(" ")));
            else if (i%4==1 && max_size == 0) seqs.push_back(line);
            else if (i%4==1) seqs.push_back(line.substr(0, max_size));
            i++;
        }
    }

    else if (format == "fasta") {
        std::cout << "handling fasta input ... " << std::endl;
        std::string seq;
        while (getline(in, line)) {
            if (line[0] == '>') {
                ids.push_back(line.substr(1,line.find(" ")));                
                if (max_size == 0) seqs.push_back(seq);
                else seqs.push_back(seq.substr(0, max_size)); 
                seq = ""; //reset seq for next record        
            }
            else seq += line; 
            i++;
        }
        //push last seq
        if (max_size == 0) seqs.push_back(seq);
        else seqs.push_back(seq.substr(0, max_size));
    }

    if (ids.size() != seqs.size()) {
        std::cout << "loading test set failed! input file might not be formatted properly" << std::endl;
        return;
    }
    for (size_t i = 0; i < ids.size(); i++) {
        test_set.emplace_back(ids[i], seqs[i]);  
    }

    munmap(f, size_);
}

