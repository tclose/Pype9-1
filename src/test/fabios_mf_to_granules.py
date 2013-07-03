#!/usr/bin/env python
"""
  This script creates and runs the Granular network that Fabio created for his 2011 paper.

  @author Tom Close

"""

#######################################################################################
#
#    Copyright 2011 Okinawa Institute of Science and Technology (OIST), Okinawa, Japan
#
#######################################################################################
import os.path
#if 'NINEMLP_MPI' in os.environ:
from mpi4py import MPI #@UnresolvedImport @UnusedImport
import argparse
import ninemlp
from pyNN.parameters import Sequence
import sys
import numpy as np
from ninemlp import create_seeds
# Set the project path for use in default parameters of the arguments
PROJECT_PATH = os.path.normpath(os.path.join(ninemlp.SRC_PATH, '..'))
# Parse the input options
DEFAULT_TIMESTEP = 0.02
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--simulator', type=str, default='neuron', help="simulator for NINEML+ (either "
                                                                    "'neuron' or 'nest')")
parser.add_argument('--build', type=str, default=ninemlp.DEFAULT_BUILD_MODE, 
                    help="Option to build the NMODL files before running (can be one of {})"
                    .format(ninemlp.BUILD_MODE_OPTIONS))
parser.add_argument('--mf_rate', type=float, default=5, help="Mean firing rate of the Mossy Fibres "
                                                             "(default: %(default)s)")
parser.add_argument('--time', type=float, default=2000.0, help="The run time of the simulation (ms)"
                                                               " (default: %(default)s)")
parser.add_argument('--output', type=str, 
                    default=os.path.join(PROJECT_PATH, 'output', 'fabios_mf_to_granule.'), 
                    help="The output location of the recording files")
parser.add_argument('--start_input', type=float, default=1000, help="The start time of the mossy "
                                                                    "fiber stimulation "
                                                                    "(default: %(default)s)")
parser.add_argument('--min_delay', type=float, default=DEFAULT_TIMESTEP, 
                    help="The minimum synaptic delay in the network (default: %(default)s)")
parser.add_argument('--timestep', type=float, default=DEFAULT_TIMESTEP, 
                    help="The timestep used for the simulation (default: %(default)s)")
parser.add_argument('--save_connections', type=str, default=None, help="A path in which to save "
                                                                       "the generated connections")
parser.add_argument('--net_seed', type=int, default=None, help="The random seed used to generate the "
                                                               "stochastic parts of the network") 
parser.add_argument('--stim_seed', type=int, default=None, help="The random seed used to generate "
                                                                " the stimulation spike train.")
parser.add_argument('--separate_seeds', action='store_true',
                    help="Instead of a constant seed being used for each process a different seed "
                         "on each process, which is required if only minimum number of generated "
                         "random numbers are generated on each node, instead of the whole set. This "
                         "means the simulation will be dependent on not just the provided seeds but "
                         "also the number of processes used, but otherwise shouldn't have any "
                         "detrimental effects")
parser.add_argument('--spike_times', nargs='+', default=[], action='append', type=int,
                    metavar=('POP_ID', 'SPIKE_TIMES'), 
                    help="The Mossy Fiber ID followed by its spike times afterwards")
parser.add_argument('--save_volt_traces', action='store_true', 
                    help="Stores the voltage traces of the post-synaptic cells that are sent spikes")
parser.add_argument('--debug_network', action='store_true', help="Loads a stripped down version of "
                                                                 "the network for easier debugging")
parser.add_argument('--silent_build', action='store_true', help="Suppresses all build output")
parser.add_argument('--log', type=str, help="Save logging information to file")
args = parser.parse_args()
# Delete all system arguments once they are parsed to avoid conflicts in NEST module
del sys.argv[1:]
# Set up logger
if args.log:
    from pyNN.utility import init_logging
    init_logging(args.log, debug=True)
# Set the network xml location
network_xml_location = os.path.join(PROJECT_PATH, 'xml/cerebellum/fabios_mf_to_granule.xml')
# Set the build mode for pyNN before importing the simulator specific modules
ninemlp.pyNN_build_mode = args.build
exec("from ninemlp.%s import *" % args.simulator)
from pyNN.random import NumpyRNG
# Set the random seeds
net_seed, stim_seed = create_seeds((args.net_seed, args.stim_seed), 
                                   simulator.state if args.separate_seeds else None) #@UndefinedVariable
net_rng = NumpyRNG(net_seed)
stim_rng = NumpyRNG(stim_seed)
if args.build != 'compile_only' or args.build != 'build_only':
    print "Random seed used to generate the stochastic elements of the network is %d" % net_seed
    print "Random seed used to generate the stimulation spike train is %d" % stim_seed    
# Build the network
print "Building network"
net = Network(network_xml_location, timestep=args.timestep, min_delay=args.min_delay, max_delay=20.0, #@UndefinedVariable
              build_mode=args.build, silent_build=args.silent_build, rng=net_rng)
print "Setting up simulation"
mossy_fibers = net.get_population('MossyFibers')
granules = net.get_population('Granules')
proj = net.get_projection('MossyFibers_Granules_AMPA')
#mossy_fibers.set_poisson_spikes(args.mf_rate, args.start_input, args.time, stim_rng.rng)
print "Setting up recorders"
net.record_spikes()
# Set up voltage traces    
spike_times = [Sequence([])] * len(mossy_fibers)
for times in args.spike_times:
    ID = times[0]
    spike_times[ID] = Sequence(times[1:])
    if args.save_volt_traces:
        conn_list = np.array(proj.get([], 'list'), dtype=int)
        post_proj = conn_list[np.where(conn_list[:,0] == ID), 1]
        local_proj = post_proj[granules._mask_local[post_proj]]
        if len(local_proj):
            granules[local_proj].record_v()
mossy_fibers.set(spike_times=spike_times)    
print "Network description"
net.describe()
if args.save_connections:
    print "Saving connections"
    net.save_connections(args.save_connections)
    print "Saving cell positions"
    net.save_positions(args.save_connections)
print "Starting run"
# Print out basic parameters of the simulation
print "Simulation time: %f" % args.time
print "Stimulation start: %f" % args.start_input
print "MossyFiber firing rate: %f" % args.mf_rate
# Actually run simulation
run(args.time) #@UndefinedVariable
print "Simulated Fabio's Network for %f milliseconds" % args.time
net.write_data(args.output)
# Save recorded data to file
#net.print_spikes(args.output)
#for volt_trace in args.volt_trace:
#    pop = net.get_population(volt_trace[0])
#    pop.write_data(args.output + volt_trace[0] + ".v.pkl", variables='v')
print "Saved recorded data to files '{}*.*'".format(args.output)