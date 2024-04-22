# KGWASMatrix: An optimized workflow for producing k-mer count matrices for large GWAS panels.
For a detailed description of the parallalization of the k-mer count pipeline, please refer to KGWAS.pdf. 

# Installing and Running the Executables
The following steps detail how to install and run the k-mer GWAS matrix generation pipeline.

First, create a workspace folder, e.g. KMGWASMatrix:

``
mkdir KGWASMatrix
``

``
cd KGWASMatrix
``

Then clone this repo into the workspace:

``
git clone git@github.com:githubcbrc/KGWASMatrix.git .
``

Next run the installation script:

``
./install.sh
``

This will build the docker image and start the container for compiling the source code (and preferably running the executables). By the time the installation finishes you should have a build folder with two main executables:
``kmer_count`` and ``matrix_merge``, which are all that is needed to run the pipeline. You can run these from here, or move them to a bin folder and them to your $PATH. For running on a HPC cluster, it is preferable to convert the docker image into singularity and run singularity instances on the cluster (using singularity exec). Running the executables directly is also possible as long as the necessary libraries are installed.

`kmer_count` operates on accession files, and expects fastq files under the `./data` folder. The accession paths are presumed to be for paired-end sequencing data files, therefore, the data folder is expected to have two files per accession, e.g.: `./data/A123_1.fq` and `./data/A123_2.fq`. To proceed, create the data folder at the root of the project, and move your sequencing data there in the format described. If the data size is large, either use a simlink or amend the ``start_container.sh`` to mount your data path into `./data`. 

`kmer_count` expects three parameters: `<accession> <number of files> <output folder path>`. `accession` is the name of the accession, e.g. A123, which would load the reads from the two files: `./data/A123_1.fq` and `./data/A123_2.fq`. `number of files` is the number of k-mer bins that would be used for sharding the k-mer index, and defines the level of parallelism for the ``matrix_merge`` phase. `output path` is the desired location for writing binned k-mer count results.

For example, `kmer_count A123 200 ./output` would load the reads of accession A123 from the `./data` folder, index the k-mer occurence using 200 bins, and write the results into the `./output` folder.


# Program Parameters

# HPC Job Examples
