#/usr/bin/env python
from mpi4py import MPI as mpi
import os
import sys

rank = mpi.COMM_WORLD.Get_rank()
nproc = mpi.COMM_WORLD.Get_size()

data_dir = '/scratch3/scratchdirs/loring/dipole3-den-isos-all/'
in_file = '%s/den-iso-%04d-0001.pvtp'%(data_dir, rank)
out_file = '%s/den-iso-%04d-0001.pov'%(data_dir+'/0001-pov-nn/', rank)
prefix = '/usr/common/graphics/ParaView/builds/vtk2pov'

cmd='%s/bin/vtk2pov --input=%s --output=%s'%(prefix, in_file, out_file)

sys.stderr.write('=====%d %s\n'%(rank, cmd))
os.system(cmd)
sys.stderr.write('=====%d complete!\n'%(rank))
