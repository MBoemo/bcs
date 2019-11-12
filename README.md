# bcs
Simulates the behaviour of systems written in the beacon calculus.  Development was done using gcc 5.4.0 on an Ubuntu 16.04 platform.

## Downloading and Compiling bcs
Clone the bcs repository by running:
```shell
git clone https://github.com/MBoemo/bcs.git
```
The bcs directory will appear in your current directory.  bcs was written in C++11 and uses OpenMP for parallel processing, but these are standard on most systems; there are no other third party dependencies.  Compile the software by running:
```shell
cd bcs
make
```
This will put the bcs executable into the bcs/bin directory.  bcs was tested to compile and run on both OSX and Linux systems.

## Manual
A manual for writing models in the beacon calculus and using bcs is available at https://www.boemogroup.org/the-beacon-calculus.  It includes:
- quick-start tutorials,
- a bank of examples,
- a full list of features.

## Citation
The Beacon Calculus language and bcs simulator are described in the following preprint posted to bioRxiv:

Boemo, M.A., Cardelli, L., Nieduszynski, C.A. (2019) The Beacon Calculus: A formal method for the flexible and concise modelling of biological systems.  bioRxiv DOI: https://doi.org/10.1101/579029

## Questions and Comments
Should any bugs arise or if you have a question about usage, please raise a GitHub issue at https://github.com/MBoemo/bcs/issues. If you have comments or suggestions to improve the Beacon Calculus, the bcs software, or the manual, please Email mb915@cam.ac.uk.
