from mpi4py import MPI
from neuron import h
pc = h.ParallelContext()
print "On process {} of {}".format(pc.id(), pc.nhost())