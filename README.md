# bcs
[![Documentation Status](https://readthedocs.org/projects/beacon-calculus-simulator/badge/?version=latest)](https://beacon-calculus-simulator.readthedocs.io/en/latest/?badge=latest)

Software that simulates the behaviour of systems written in the Beacon Calculus.  bcs is under development by the [Boemo Group](https://www.boemogroup.org/) at the University of Cambridge.

## Downloading and Compiling bcs
Clone the bcs repository by running:
```shell
git clone https://github.com/MBoemo/bcs.git
```
The bcs directory will appear in your current directory.  bcs was written in C++11 and uses OpenMP for parallel processing, but these are standard on most systems; there are no other third party dependencies.  Compile the latest version of bcs by running:
```shell
cd bcs
git checkout v1.0.0
make
```
This will put the bcs executable into the bcs/bin directory.  Development was done using gcc 5.4.0 on an Ubuntu 16.04 platform, and bcs was tested to compile and run on both OSX and Linux systems.

## Documentation and Manual
A manual for writing models in the Beacon Calculus and documentation for bcs is available at https://beacon-calculus-simulator.readthedocs.io.  It includes:
- quick-start tutorials,
- a bank of examples,
- a full list of features.

## Citation
Please cite the following publication if you use the Beacon Calculus for your research:
Boemo MA, Cardelli L, Nieduszynski CA. The Beacon Calculus: A formal method for the flexible and concise modelling of biological systems. *PLoS Computational Biology* 2020;16:e1007651. [[bioRxiv](https://doi.org/10.1101/579029)] [[Journal Link](https://doi.org/10.1371/journal.pcbi.1007651)]

## Questions and Comments
Should any bugs arise or if you have a question about usage, please raise a GitHub issue at https://github.com/MBoemo/bcs/issues. For more detailed discussions on the Beacon Calculus, the bcs software, the manual, or for collaborations, please Email mb915@cam.ac.uk.
