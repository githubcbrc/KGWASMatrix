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

`kmer_count` operates on accession files


# Program Parameters

# HPC Job Examples
