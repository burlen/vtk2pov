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

Testing
=======
To run the test execute the command on the included test data
and launch povray using the included script.

    ./bin/vtk2pov --input=./data/cone.pvtp --output=./data/cone.pov --verbose --subdivision=loop --ndivisions=4
    povray pov.ini
