
# KGWASMatrix

**KGWASMatrix** is an optimized workflow designed for producing k-mer count matrices for large GWAS panels. For an in-depth explanation of the parallelization strategy for the k-mer counting pipeline, please consult the `KGWAS.pdf` document included in this repository.


## How to Cite

If you find this work useful, please cite: Emile Cavalet-Giorsa, Andrea González-Muñoz, et al bioRxiv 2023.11.29.568958; doi: https://doi.org/10.1101/2023.11.29.568958

## Installation

### Prerequisites
#### **Docker: 
For setting up an insulated environment for the compilation and execution of the codes, a Docker container is provided. Make sure you have Docker engine installed on your host.  See the `Dockerfile` for a container setup that includes all necessary dependencies.
#### Singularity:(Strongly Recommanded) 
Singularity will be needed to convert the Docker image into singularity for running on HPC (see below), this would most likely already be installed on your HPC cluster.



To install and set up the KGWASMatrix pipeline, follow these steps:

### Environment Setup

1. **Create and navigate to your workspace directory:**
   ```bash
   mkdir KGWASMatrix
   cd KGWASMatrix
   ```


2. **Clone the github repository:**
   ```bash
   git clone git@github.com:githubcbrc/KGWASMatrix.git .
   ```

3. **Run the installation script:**

   ```bash
   ./install.sh
   ```

If you inspect the installation script, you can see that all it does is build a docker image, and spin up a container for compiling the source code (and preferably running the executables).
   ```bash
   #!/bin/bash
   cd init
   bash ../scripts/build_img.sh #--no-cache
   bash ../scripts/start_cont.sh
   docker exec -it gwascont bash /project/scripts/build_binaries.sh
   ```
After the installation the ``./build`` folder will contain two key executables:``kmer_count`` and ``matrix_merge``, which are all that is needed to run the pipeline. You have the option to run these executables directly from their current location, or you may choose to relocate them to a ``bin`` directory and include this directory in your system's ``$PATH`` for easier access. For execution on High-Performance Computing (HPC) clusters, it is recommended to transform the Docker image into a Singularity image. This conversion allows you to run the pipeline using ``singularity exec``. Alternatively, the executables can be run directly on the cluster, provided all the necessary libraries are installed.  
Use the following command to convert the Docker image into Singularity:
```bash
singularity build gwasimg.sif docker-daemon://gwasimg
```


## Data Preparation

### Data Path
The ``kmer_count`` executable operates on accession files and requires FASTQ files to be located in the ``./data`` folder. The current setup primarily supports paired-end sequencing data files, (mainly because this was our use case). For each accession, you should have two files: for example, ``./data/A123_1.fq`` and ``./data/A123_2.fq`` for accession ``A123``. In the future, we plan to expand the utility's flexibility by introducing additional options to accommodate a wider range of sequencing data types.

 To proceed, establish a ``./data`` directory at the root of the project, and move your sequencing data into this directory, maintaining the format specified above. 
```bash
mkdir ./data
mv path_to_your_sequencing_data/* ./data/
```
If the data size is large, either consider using a symbolic link, 
```bash
ln -s /path_to_large_data ./data
```


Or, amending the ``start_container.sh`` script to mount your external data path directly into the ``./data`` directory within the container by adding another volume mapping to the ``docker run`` command. Here is how you can adjust the ``docker run`` command to include a specific data volume:

```bash
id=$(docker run --rm -d --name ${cont} -it -v $projectDir:/project -v $dataDir:/project/data ${img})
```
In this command:

``$dataDir`` represents the path on your host system where your data is stored that you want to be accessible from within the container.
``/project/data`` is the path inside the container where this data will be accessible. This corresponds to the expected location where ``kmer_count`` will look for FASTQ files. This latter option is not practical for HPC scenarios, but in such cases, data transfer is usually unavoidable.

### File Naming
FASTQ files must be:
* placed inside the ./data directory,
* be uncompressed
* named as follows: **${accession}_1.fq** or **${accession}_2.fq** where:
  - ${accession} refers to a unique name of an accession from the panel of accessions under study
  - _1 and _2 are the forward and reverse short reads
  - FASTQ file extension must be **.fq**

**Suggestion:**
If storage is a limitation, consider decompressing the FASTQ files on-demand.

### List of Accessions
Names of accessions should be kept in a text file e.g. _accessions.txt_, one name per line. Each name should have a matching pair of FASTQ files (as per the naming format above).

## Usage Instructions
### K-mer Counting
#### Command:
```bash
kmer_count <accession> <number of bins> <output folder>
```
#### Example:
```bash
kmer_count A123 200 ./output
```

The ``kmer_count`` tool requires three parameters to operate: ``<accession>``, ``<number of bins>``, and ``<output folder>``.

1. **accession:** This parameter specifies the name of the accession (e.g., A123). It is used to identify and load the corresponding paired-end sequencing data files, e.g. ``./data/A123_1.fq`` and ``./data/A123_2.fq``.
2. **number of bins:** This indicates how many k-mer bins to create, which are used to shard the k-mer index. This number directly influences the granularity of parallelism during the ``matrix_merge`` phase.
3. **output folder:** This is the directory where the results of the binned k-mer counts will be stored. It should be specified as a path relative to the root of the project or an absolute path.

**Note:** a k-mer size of 51 is used for this study, you may change the value of k in the ``./src/kmer_count.cpp`` and recompile the code by running `./install` again.

#### Singleton k-mers
By default, k-mers that occur only once in the count are considered of error origin and are discarded by default.

#### Multi-threading

Computation is accelerated by splitting the accession files into `NUM_CHUNKS` chunks, creating a task per chunk, and running a thread pool to execute the tasks. So, ``NUM_CHUNKS`` governs the granularity of the parallelism for this phase, but the level of parallelism is defined by the number of threads in the pool (the current code uses all available threads on a computational node to maximize resource utilisation, but users can tweak that if they so wish). While ``NUM_CHUNKS`` is currently hard-coded to optimize performance through static array usage, future revisions may introduce dynamic configurations to increase flexibility. Access to the files is synchronized using a mutex array.

#### Output Format:
For example, `kmer_count A123 200 ./output` would load the reads of accession A123 from the `./data` folder, index the k-mer occurence using 200 bins, and write the results into the `./output` folder. This will create 200 files, one accession index per bin:

```bash
./output/A123/1_nr.tsv
./output/A123/2_nr.tsv
...
./output/A123/200_nr.tsv
```

Each <index>_nr.tsv file has the following format:
```
<kmer_of_length_x><tab><count>
```
e.g. 
```
ATCGTAGCTGATGCAAGAGGGCCCTGGATTAGGAGAGCGTTGGAGAGCTG    21
```

### Merging K-mer Bins 
#### Command:
```bash
matrix_merge <input path> <accessions list> <bin index> <min occurrence threshold>
```
#### Example:
```bash
matrix_merge ./output accessions.txt 35 6
```

Once k-mer counts are done, all is left is to merge all the bins with the same index into a "matrix bin", and for that we use ``matrix_merge``. In a nutshell, `matrix_merge` takes all the files with the same name from different accessions, and merges them into one index.

`matrix_merge` expects the following parameters:

1. **input path:** Specifies the directory where the binned k-mer counts are stored. Typically, this is the output directory from the ``kmer_count`` phase.
2. **accessions path:** The path to a text file listing all accession names, one per line. Example file format can be found in ``accessions.txt`` included in this repository.
3. **file index:** Indicates the specific bin number to merge in this operation.
4. **min occurrence threshold:** Defines the minimum and the maximum k-mer frequency in the panel. This frequency does not refer to the k-mer count in any specific accession but the k-mer frequency across the panel. K-mers with a frequency below this threshold are excluded (`frequency < minimum threshold`). And k-mers with a frequency exceeding the complement of this threshold relative to the panel size are also excluded (i.e., `frequency > panel size - minimum threshold`).

**Note**: in ``matrix_merge.cpp``, ``NUM_ACC`` is a hard-coded parameter specifying the number of accessions to be processed. Make sure to recompile the code with your number of accessions by updating the value and re-running `./install`. 

Let's say we have a panel of 1000 accessions for which the k-mer count was performed for each accession using the command `kmer_count <accession> 200 ./output`. To merge the k-mer counts and create a matrix, we run the following `matrix_merge ./output accessions.txt <index> 6` on each of the 200 bins (zero indexed i.e. 0-199). For example, `matrix_merge ./output accessions.txt 35 6` will look for all 1000 k-mer count files in `./output/<accessions>/35_nr.tsv` with names in `accessions.txt`, and merge all bins with index 35: 

```bash
./output/A000/35_nr.tsv
./output/A001/35_nr.tsv
...
./output/A999/35_nr.tsv
```
This creates a matrix with binary values representing k-mer presence/absence (using a k-mer minimum occurence of 6 for binarizing the index), which gets saved under `matrix_6/35_m.tsv`. Concatenating these partial matrices results in the full k-mer GWAS matrix.

#### Output:
* The output path from `matrix_merge` is saved under `matrix_<min_occurrence_threshold>`.
* Matrix files will have the following naming format: `matrix_<min_occurrence_threshold>/<index>_m.tsv`
* Number of `<index>_m.tsv` should match the number of bins that was specified in the earlier k-mer count command `kmer_count <accession> <number of bins> <output folder>`
* Each `<index>_m.tsv` has the format `<kmer_string><tab><accessionA 0|1><accessionB 0|1>...<accessionN 0|1>` where order of accessions (columns) is matching that in accessions.txt.
   For example:
  ```bash
  > head -n1 matrix_6/35_m.tsv
  > ATCGTAGCTGATGCAAGAGGGCCCTGGATTAGGAGAGCGTTGGAGAGCTG    0011001100001111100010101...1
  ```

## HPC Job Examples

### K-mer count

Here is a basic SLURM job script example for running the ``kmer_count`` command on an HPC cluster. 

```bash
#!/bin/bash
#SBATCH --job-name=kmer_count_job_<accession>       # Job name
#SBATCH --nodes=1                                   # Number of nodes. Hint: keep this to 1
#SBATCH --mem=256G                                  # Memory needed per node. Hint: the more RAM is the better/safer
#SBATCH --time=10:00:00                             # Time limit day-hrs:min:sec
#SBATCH --output=kmer_count_<accession>_%j.log      # Standard output and error log
#SBATCH --cpus-per-task=40                          # cpus

# Set environment variables (if needed)
export OMP_NUM_THREADS=$(nproc)

# Move to the directory where the data is located (edit accordingly)
cd /your/project/directory

# Check if all parameters are provided
if [ "$#" -ne 3 ]; then
    echo "Usage: sbatch $0 <accession> <number of bins> <output directory>"
    exit 1
fi

# Access command-line arguments
ACCESSION=$1
NUM_BINS=$2
OUTPUT_DIR=$3

# Run kmer_count
./build/kmer_count $ACCESSION $NUM_BINS $OUTPUT_DIR

# Note: Run this script by submitting it through SLURM with:
# sbatch submit_kmer_count.sh A123 200 ./output
```

**Script for Submitting Jobs for All Accessions (submit_all_accessions.sh)**

Suppose you have a file named ``accessions.txt`` where each line contains an accession name. You can write a wrapper script that reads each accession from the file and submits a job for it.

```bash
#!/bin/bash

# Loop through each line in accessions.txt
while IFS= read -r accession
do
  sbatch submit_kmer_count.sh "$accession" 200 ./output
done < "accessions.txt"
```

### Matrix merge

Here is an example of a SLURM job script for running the ``matrix_merge`` command. 
```bash
#!/bin/bash
#SBATCH --job-name=matrix_merge_job   # Job name
#SBATCH --nodes=1                     # Number of nodes
#SBATCH --mem=256G                    # Memory per node
#SBATCH --time=05:00:00               # Time limit hrs:min:sec
#SBATCH --output=matrix_merge_%j.log  # Standard output and error log

# Assuming matrix_merge is already built and available under ./build/
cd /your/project/directory

# Ensure the command line parameters are passed
if [ "$#" -ne 4 ]; then
  echo "Usage: sbatch $0 <input path> <accessions path> <bin index> <min occurrence threshold>"
  exit 1
fi

# Access command-line arguments
INPUT_PATH=$1
ACCESSIONS_PATH=$2
FILE_INDEX=$3
MIN_OCCURRENCE_THRESHOLD=$4

# Run matrix_merge
./build/matrix_merge $INPUT_PATH $ACCESSIONS_PATH $FILE_INDEX $MIN_OCCURRENCE_THRESHOLD

# Example command usage:
# sbatch submit_matrix_merge.sh ./output accessions.txt 35 6
```

**Script for Submitting Jobs for All Bins (submit_all_bins.sh)**

This script will take parameters for the input path, accessions list, and minimum occurrence threshold, and then loop through each bin index to submit a job.
```bash
#!/bin/bash

# Check for required command line arguments
if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <input path> <accessions path> <min occurrence threshold>"
  exit 1
fi

# Assign command-line arguments to variables
INPUT_PATH=$1
ACCESSIONS_PATH=$2
MIN_OCCURRENCE_THRESHOLD=$3

# Number of bins - adjust this according to your specific setup
TOTAL_BINS=200

# Loop over each bin index
for (( bin=0; bin<TOTAL_BINS; bin++ ))
do
  # Submit a SLURM job for each bin
  sbatch submit_matrix_merge.sh $INPUT_PATH $ACCESSIONS_PATH $bin $MIN_OCCURRENCE_THRESHOLD
done

echo "Submitted matrix merge jobs for all $TOTAL_BINS bins."
```

## Hardware and performance considerations
You should take the following considerations into account:

### IO considerations
The k-mer count step is highly IO intensive, especially if multiple jobs are executed in a shared HPC environment with a network file system. We strongly recommend the use of highly-performant storage solutions where available such as NVME SSDs and Burst Buffers. Traditional spinning hard drives will struggle to keep up with the high IO. Where fast storage is not available, we recommend the use of local scratch on the compute nodes to distrbute the IO workload.

**Hint:** start small, assess then increase.


### Memory considerations
Memory is another posssible bottleneck for both the `kmer_cout` step and the `matrix_merge` step.

#### k-mer counting stage:

The genome size and sequencing depth are two deciding factors for RAM utilisation in the kmer_count step. As genome size increases and sequencing depth increases ( typically up to ~ 10x ), the size of the FASTQ files will also increase and has to fit in RAM along with computed k-mers and counts.

#### matrix merging stage:

Genome size will also have a direct impact on the memory utilisation at this stage. The other factor is the number of accessions in the panel. As the number of panels increases, the total number of k-mer count bins also increases and hence the RAM footprint will increase. Furthermore, bigger genomes are likely to have more k-mers compared with smaller genomes and with more variants sequenced, variant k-mers are also more likely to occur.

To mitigate these bottlenecks, you should consider increasing the number of bins to spread k-mers across as many files as possible. The k-mer binning should ensure the bins are roughly even.  However, you should also consider that the more file handles are opened, the more IO strain on the storage.

