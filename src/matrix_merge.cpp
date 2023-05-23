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
const size_t NUM_ACC = 201;
vector<string> split(const string &str, const char &sep)
{
    string segment;
    vector<string> ret;
    istringstream ss(str);
    while (getline(ss, segment, sep))
        ret.push_back(segment);
    return ret;
}

vector<string> get_accessions(string accessions_path)
{
        vector<string> accessions;
	cout << "accessions path: " << accessions_path << endl;
	try {
        	ifstream stream(accessions_path);
        	string line;
        	while (getline(stream, line))
        	    accessions.push_back(line);
        	stream.close();
	}
        catch (exception const &e)
        	{cout << e.what() << endl;}
	if (accessions.size() == 0) 
		cout << "could not read accessions, make sure file is not empty." << endl;
	return accessions;
}

void merge_chunk(const uint file_index, const uint min_occur, string input_path, string accessions_path)
{ 

    auto accessions = get_accessions(accessions_path);
    unordered_map<string, ushort[NUM_ACC+1]> matrix_;
    size_t acc_index = 0;	

    for (auto &acc : accessions)
    {
        string file_path = input_path + acc + "/" + to_string(file_index) + "_nr.tsv";
	if (!filesystem::exists(file_path)) {cout << "file: " << file_path << " doe not exist!" << endl;}
        ifstream stream(file_path);
        string line;
        while (getline(stream, line))
        {
            auto record = split(line, '\t');
            if (record.size() != 2)
                throw runtime_error("Could not open kmer count files!");
            string key = record[0];
            ushort value = stoi(record[1]);
            matrix_[key][acc_index] = value;
            matrix_[key][NUM_ACC]++;
	    	
        }
        stream.close();
        acc_index++;
    }

    /* --frequency version - enable when needed!
    ofstream m_stream("matrix/" + to_string(file_index) + "_m.tsv");
    for (auto & pair_ : matrix_) {
	if (pair_.second[NUM_ACC] < min_occur || pair_.second[NUM_ACC] > NUM_ACC - min_occur ) continue;
        m_stream << pair_.first;
        for (auto & freq : pair_.second)
            m_stream << "\t" << freq;
        m_stream << "\n";    
    }*/


    ofstream m_stream("matrix_"+to_string(min_occur)+"/" + to_string(file_index) + "_m.tsv");
    for (auto & pair_ : matrix_) {
	if (pair_.second[NUM_ACC] < min_occur || pair_.second[NUM_ACC] > NUM_ACC - min_occur ) continue;
        m_stream << pair_.first << "\t";
        auto freqs = pair_.second;
        for (int i = 0; i < NUM_ACC ; i++) {
            auto freq = freqs[i];
            if (freq > 0) freq = 1;
            m_stream << freq;
        }
            
        m_stream << "\n";    
    }
            
    m_stream.close();
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        cout << "usage: " << argv[0] << " <input path> <accessions path> <file index> <min occurence threshold (across panel)>\n";
        return -1;
    }

    try
    {
      	string input_path = argv[1];
	string accessions_path = argv[2];
        uint file_index = stoi(argv[3]);
	uint min_occur = stoi(argv[4]);
      	if (input_path[input_path.size()-1] != '/')
                input_path += '/';

	if (!filesystem::exists("matrix_"+to_string(min_occur)))
      		filesystem::create_directory("./matrix_"+to_string(min_occur)+"/");

        cout << "***************************** " << endl;
        cout << "PROCESSING MATRIX CHUNK: " << file_index << endl;
        auto start = chrono::steady_clock::now();

        merge_chunk(file_index, min_occur, input_path, accessions_path);

        auto end = chrono::steady_clock::now();
        cout << "processing index: " << file_index << "took "
             << chrono::duration_cast<chrono::seconds>(end - start).count()
             << " sec" << endl;

        cout << "FINISHED MATRIX CHUNK: " << file_index << endl;
        cout << "***************************** " << endl;
    }

    catch (exception const &e)
    {
        cout << e.what() << endl;
    }
}

