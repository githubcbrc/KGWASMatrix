#include "thread_pool.hpp"
#include <iostream>
#include <istream>
#include <streambuf>
#include <fstream>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <mutex>
#include <string>
#include <cstring>
#include <chrono>
#include "mmap_io.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>

using namespace std;

const size_t NUM_CHUNKS = 200000;
const int k = 51;

const map<char, string> bits = {
    {'A', "00"},
    {'C', "01"},
    {'G', "10"},
    {'T', "11"}};

const map<string, char> bases = {
    {"00", 'A'},
    {"01", 'C'},
    {"10", 'G'},
    {"11", 'T'}};

struct membuf : streambuf
{
   membuf(char *begin, char *end)
   {
      this->setg(begin, begin, end);
   }
};

bitset<2*k> bit_encode(string kmer)
{
   string code = "";
   for (auto &c : kmer)
      code += bits.at(c);
   return bitset<2*k>(code);
}

string bit_decode(bitset<2*k> code)
{
   string kmer = "";
   string bcode = code.to_string();
   for (size_t i = 0; i < bcode.size(); i += 2)
      kmer += bases.at(bcode.substr(i, 2));
   return kmer;
}

class Accession
{
public:
   vector<string> reads;
   const string accession_path;

   Accession(const string &accession_path) : accession_path(accession_path){};

   void load_reads()
   {
      string fname = accession_path;
      int fd = open(fname.c_str(), O_RDONLY);
      if (fd == -1)
         handle_error("open");

      struct stat sb;
      if (fstat(fd, &sb) == -1)
         handle_error("fstat");

      size_t size_ = sb.st_size;

      char *f = static_cast<char *>(mmap(NULL, size_, PROT_READ, MAP_PRIVATE, fd, 0u));
      if (f == MAP_FAILED)
         handle_error("mmap");

      auto l = f + size_;
      membuf sbuf(f, l);
      istream in(&sbuf);
      string line;

      reads.reserve((int)1e6);

      string format;
      getline(in, line);
      if (line[0] == '>')
         format = "fasta";
      else if (line[0] == '@')
         format = "fastq";
      else
         handle_error("unrecognized file format");
      size_t i = 1;

      cout << "handling fastq input ... " << endl;
      while (getline(in, line))
      {
         if (i % 4 == 1)
            reads.push_back(line);
         i++;
      }

      munmap(f, size_);
   }
};

string canonical(const string &seq)
{
   string ret = "";
   for (auto it = seq.rbegin(); it != seq.rend(); it++)
   {
      switch (*it)
      {
      case 'A':
         ret.push_back('T');
         break;
      case 'C':
         ret.push_back('G');
         break;
      case 'G':
         ret.push_back('C');
         break;
      case 'T':
         ret.push_back('A');
         break;
      default:
         return "";
      }
   }
   if (ret < seq)
      return ret;
   return seq;
}

class kmers_obj
{
public:
   unordered_map<bitset<2*k>, ushort> kmers_;
   size_t chunk_index;
   vector<string> chunk;
   kmers_obj(size_t chunk_index, vector<string> &chunk) : chunk_index(chunk_index), chunk(chunk)
   {
      cout << "Running Chunk: " << chunk_index << "/" << NUM_CHUNKS << endl;
   };

   void index(ofstream *key_streams, ofstream *value_streams, mutex *my_mutex, size_t num_files)
   {
      string kmer;
      for (auto &seq : chunk)
      {
         for (size_t i = 0; i < seq.size() - k + 1; i++)
         {
            kmer = seq.substr(i, k);
            kmer = canonical(kmer);
            auto code = bit_encode(kmer);
            kmers_[code]++;
         }
      }

      for (auto &pair_ : kmers_)
      {
         auto key = pair_.first;
         auto value = pair_.second;
	 auto key_hash = bitset<2*k>(key.to_string().substr(0,64)).to_ulong();
         auto idx = key_hash % num_files;
         lock_guard lock(my_mutex[idx]);
         key_streams[idx].write(reinterpret_cast<char*>(&key), sizeof(key));
         value_streams[idx].write(reinterpret_cast<char*>(&value), sizeof(value));
      }
   }
};

vector<string> split(const string &str, const char &sep)
{
   string segment;
   vector<string> ret;
   istringstream ss(str);
   while (getline(ss, segment, sep))
      ret.push_back(segment);
   return ret;
}

void dedup_chunk(const string file_path)
{
	string keys_path = file_path + "/keys.dat";
	string values_path = file_path + "/values.dat";	
	
	ifstream keys_f(keys_path, ios::in | ios::binary);
	ifstream values_f(values_path, ios::in | ios::binary);
		
	keys_f.seekg(0, keys_f.end);
	auto kN = keys_f.tellg();              
	keys_f.seekg(0, keys_f.beg);
	
	values_f.seekg(0, values_f.end);
	auto vN = values_f.tellg();              
	values_f.seekg(0, values_f.beg);
	
	vector<bitset<2*k>> keys(kN / sizeof(bitset<2*k>));
	vector<ushort> values(vN / sizeof(ushort));
	
	keys_f.read(reinterpret_cast<char*>(keys.data()), keys.size()*sizeof(bitset<2*k>));
	values_f.read(reinterpret_cast<char*>(values.data()), values.size()*sizeof(ushort));
	
	keys_f.close();
	values_f.close();

	unordered_map<bitset<2*k>, ushort> kmers_;

    	for(size_t i = 0; i < keys.size(); i++)
        	kmers_[keys[i]] += values[i];

   	ofstream m_stream(file_path + "_nr.tsv");
   	for (auto &pair_ : kmers_)
      		if (pair_.first != 0 && pair_.second != 1)
         		m_stream << bit_decode(pair_.first) << "\t" << pair_.second << '\n';
   	m_stream.close();
   	filesystem::remove_all(file_path); 
}

int main(int argc, char *argv[])
{
   if (argc != 4)
   {
      cout << "usage: " << argv[0] << " <accession> <number of files> <output folder path>\n";
      return -1;
   }

   try
   {
      string accession = argv[1];
      size_t NUM_FILES = atoi(argv[2]);
      string output_path = argv[3];
      if (output_path[output_path.size()-1] != '/')
		output_path += '/';
	
      string accession_list[2];
      accession_list[0] = "./data/" + accession + "_1.fq";
      accession_list[1] = "./data/" + accession + "_2.fq";


      mutex *my_mutex = new mutex[NUM_FILES];

      cout << "***************************** " << endl;
      cout << "PROCESSING ACCESSION: " << accession << endl;
      auto start = chrono::steady_clock::now();


      //#################################
      vector<string> *chunks = new vector<string>[NUM_CHUNKS];
      for (auto & accession_path : accession_list)
      {
         cout << "+++ PROCESSING " << accession_path << endl;
         auto accession_obj = new Accession(accession_path);
         accession_obj->load_reads();
         cout << "num reads: " << accession_obj->reads.size() << endl;
         cout << "splitting file into " << NUM_CHUNKS << "chunks" << endl;

         for (size_t i = 0; i < accession_obj->reads.size(); i++)
         {
            auto idx = i % NUM_CHUNKS;
            chunks[idx].push_back(accession_obj->reads[i]);
         }
	delete accession_obj;
         cout << "+++++++++++++++++++++" << endl;
      }
      auto end = chrono::steady_clock::now();
      cout << "loading & chunking time in seconds: "
           << chrono::duration_cast<chrono::seconds>(end - start).count()
           << " sec" << endl;
      //#################################
      filesystem::remove_all(output_path + accession);
      filesystem::create_directory(output_path);
      filesystem::create_directory(output_path + accession);

      ofstream *key_streams = new ofstream[NUM_FILES];
      ofstream *value_streams = new ofstream[NUM_FILES];
      for (size_t i = 0; i < NUM_FILES; i++) {
         auto fni = output_path + accession + "/" + to_string(i);
         filesystem::create_directory(fni);
         key_streams[i].open(fni + "/keys.dat", std::ios::app | std::ios::binary);
         value_streams[i].open(fni + "/values.dat", std::ios::app | std::ios::binary);
      }


      thread_pool pool;

      start = chrono::steady_clock::now();

      cout << "building Kmer index ... will take long" << endl;

      for (size_t i = 0; i < NUM_CHUNKS; i++)
      {
         auto &chunk = chunks[i];
         pool.push_task([i, NUM_FILES, &chunk, &key_streams, &value_streams,  &my_mutex]
                        {
                           auto ko = new kmers_obj(i, chunk);
                           ko->index(key_streams, value_streams, my_mutex, NUM_FILES);
                           delete ko; });
      }

      pool.wait_for_tasks();
      delete [] chunks;	

      for (size_t i = 0; i < NUM_FILES; i++) {
         key_streams[i].close();
         value_streams[i].close();
      }

      end = chrono::steady_clock::now();
      cout << "building Kmer Index time: "
           << chrono::duration_cast<chrono::seconds>(end - start).count()
           << " sec" << endl;

      // merging
      start = chrono::steady_clock::now();
      cout << "+++++ Deduplicating ... " << endl;

      string path = output_path + accession, fn;
      int i = 0;
      for (const auto &f : filesystem::directory_iterator(path))
      {
         fn = f.path();
         pool.push_task([i, fn]
                        { dedup_chunk(fn); });
         cout << "Deduping " << fn << endl;
         i++;
      }

      pool.wait_for_tasks();
      end = chrono::steady_clock::now();
      cout << "Deduplicating took: "
           << chrono::duration_cast<chrono::seconds>(end - start).count()
           << " sec" << endl;


      delete[] my_mutex;
      delete[] key_streams;
      delete[] value_streams;

      cout << "FINISHED ACCESSION: " << accession << endl;
      cout << "***************************** " << endl;
   }

   catch (exception const &e)
   {
      cout << e.what() << endl;
   }

   return 0;
}
