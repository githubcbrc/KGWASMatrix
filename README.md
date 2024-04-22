
# KGWASMatrix

**KGWASMatrix** is an optimized workflow designed for producing k-mer count matrices for large GWAS panels. For an in-depth explanation of the parallelization strategy for the k-mer counting pipeline, please consult the `KGWAS.pdf` document included in this repository.

## Installation

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

If you inspect the installation script, you can see that all is does is build a docker image, and spin up a container for compiling the source code (and preferably running the executables).
   ```bash
   #!/bin/bash
   cd init
   bash ../scripts/build_img.sh #--no-cache
   bash ../scripts/start_cont.sh
   docker exec -it gwascont bash /project/scripts/build_binaries.sh
   ```
By the time the installation finishes you should have a ``./build`` folder with two main executables:
``kmer_count`` and ``matrix_merge``, which are all that is needed to run the pipeline. You can run these from here, or move them to a ``bin`` folder and add them to your $PATH. For running on a HPC cluster, it is preferable to convert the docker image into singularity and run singularity instances on the cluster (using ``singularity exec``). Running the executables directly is also possible as long as the necessary libraries are installed.

`kmer_count` operates on accession files, and expects ``fastq`` files under the `./data` folder. The accession paths are presumed to be for paired-end sequencing data files (mainly because this was our use case, but we intend to accomodate more cases in the future by exposing a number of options to the user). Therefore, the data folder is expected to have two files per accession, e.g.: `./data/A123_1.fq` and `./data/A123_2.fq`. To proceed, create the data folder at the root of the project, and move your sequencing data there in the format described. If the data size is large, either use a simlink or amend the ``start_container.sh`` script to mount your data path into `./data`. 

`kmer_count` expects three parameters: `<accession>, <number of files>, and <output folder path>`. `accession` is the name of the accession, e.g. A123, which would load the reads from the two files: `./data/A123_1.fq` and `./data/A123_2.fq` as mentioned. `number of files` is the number of k-mer bins that would be used for sharding the k-mer index, and defines the granularity of parallelism for the ``matrix_merge`` phase. `output path` is the desired location for writing binned k-mer count results.

For example, `kmer_count A123 200 ./output` would load the reads of accession A123 from the `./data` folder, index the k-mer occurence using 200 bins, and write the results into the `./output` folder. This will create 200 files, one accession index per bin:

```
./output/A123/1_nr.tsv
./output/A123/2_nr.tsv
...
./output/A123/200_nr.tsv
```

These are produced in parallel by splitting the accession files into `NUM_CHUNKS` chunks, creating a task per chunk, and running a thread pool to execute the tasks. So, NUM_CHUNKS governs the granularity of the parallelism for this phase, but the level of parallelism is defined by the number of threads in the pool (the current code uses all available threads on a computational node), users can tweak that if they so wish. `NUM_CHUNKS` is currently hard-coded for performance reasons (use of arrays instead of vectors), but this will be revised in the future. Access to the files is synchronized using a mutex array.

Once k-mer counts are done, all is left is to merge all the bins with the same index into a "matrix bin", and for that we use ``matrix_merge``. In a nutshell, `matrix_merge` takes all the files with the same name from different accessions, and merges them into one index.

`matrix_merge` expects the following parameters: `<input path>, <accessions path>, <file index>, and <min occurence threshold (across panel)>`. `input path` is just the output path from the previous phase, or wherever the binned k-mers count index is saved. `accessions path` is the path to the file containing all accessions names, one per line (see ``accessions.txt`` for an example). `file index` is the bin number for the current merge. `min occurence threshold` is the minimum k-mer frequency for considering a k-mer present in the accession. As a matter of fact, this is a bit of misnomer as it is applied symmetrically, in the sense that k-mers with ``frequency < minimum threshold || frequency > panel size - minimum threshold`` have their counts set to zero against the corresponding accession.

For example, `matrix_merge ./output accessions.txt 35 6` will look for all folders under `./output` with names in `accessions.txt`, and merge all bins with index 35:  
```
./output/A100/35_nr.tsv
./output/A101/35_nr.tsv
...
./output/A123/35_nr.tsv
```
This creates a binary matrix (using a k-mer minimum occurence of 6 for establishing presence), which is saved under `matrix_6/35_m.tsv`. Concatinating these results in the full k-mer GWAS matrix.






# HPC Job Examples
