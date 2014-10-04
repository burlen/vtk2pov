vtk2pov
=======
vtk2pov a command line converter for converting from VTK
polydata to POVRay mesh2. Input surface is triangulated and
it can optionally be subdivided using linear, loop or butterfly
filter, and normals can be optionally generated, or removed.

Build & Install
===============
Building is done through cmake + make

    mkdir bin
    cd bin
    cmake -DVTK_DIR=<path to VTK> -DENABLE_BOOST=ON ../
    make

INstalling is as simple as putting the vtk2pov in your PATH,
and ensuring your VTK install is in LD_LIBRARY_PATH.

Running
=======
The converter may be run from the command line with a number of
options set. The --input and --output options are required. Use
--help to get a secription of all the options.


Output format
=============
There are two options for output, mesh2 and mesh3. mesh3 is a
hacked binary format that can only be used with a patched version
of povray 3.7.1. The patch is included in the povray directory.
Patching povary can be done as follows:

    git clone https://github.com/POV-Ray/povray.git
    cd povray
    git checkout 7086fa93080acea
    git am -3 0001-povray-mesh3-format.patch

Now recompile and install the patched povray. This will enable mesh3
format and adds timing to both mesh2 and mesh3 parsing.

Testing
=======
To run the test execute the command on the included test data
and launch povray using the included script.

    ./bin/vtk2pov --input=./data/cone.pvtp --output=./data/cone.pov --verbose --subdivision=loop --ndivisions=4
    povray pov.ini

Parallel
========
When large numbers of files need to be converted parallel
operation may be necessary. An example of how this is done
can be found in driver.py and driver.qsub scripts. On the
Cray XC31 at NERSC submit the job as follows:

    qsub -V -A mpccc -q premium -l walltime=01:30:00 -l mppwidth=1824 driver.qsub
