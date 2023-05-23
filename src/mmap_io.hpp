#ifndef mmap_io_h
#define mmap_io_h

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

void get_reads(std::string fname, std::vector<std::pair<std::string, std::string>>& test_set, size_t max_size = 0);
template <typename T>
void read_vector(std::string fname, std::vector<T>& v);
template <typename T>
void write_vector(std::string fname, std::vector<T> v);
void handle_error(std::string msg);


template <typename T>
void write_vector(std::string fname, std::vector<T> v) {
    const size_t size_ = v.size()*sizeof(T);
    mode_t mode = S_IRUSR | S_IWUSR;
    int fd = open(fname.c_str(), O_RDWR | O_CREAT, mode);
    posix_fallocate(fd, 0, size_);

    if (fd == -1)
        handle_error("open");

    T* addr = static_cast<T*>(mmap(NULL, size_, PROT_WRITE, MAP_SHARED, fd, 0u));

    if (addr == MAP_FAILED)
        handle_error("mmap"); 

    for (int i=0; i<v.size();i++) {
        addr[i] = v[i]; 
    }
    
    msync(addr, size_, MS_SYNC);
    munmap(addr, size_);       

}


template <typename T>
void read_vector(std::string fname, std::vector<T>& v) {

    int fd = open(fname.c_str(), O_RDONLY);
    if (fd == -1)
        handle_error("open");

    struct stat sb;
    if (fstat(fd, &sb) == -1)
        handle_error("fstat");

    auto length = sb.st_size;
    //std::cout << "file size: " << length << std::endl;

    T* addr = static_cast<T*>(mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0u));
    if (addr == MAP_FAILED)
        handle_error("mmap");

    ulong N = length/sizeof(T);
    v.reserve(N);

    //std::cout << "N : " << N << std::endl;
    for (int i=0; i<N;i++) {
        v.push_back(addr[i]); 
    }

    munmap(addr, length);    

}





#endif



