import PsiMod
import re
import os
import input
import math
import warnings
from driver import *
from wrappers import *
from molutil import *
from text import *
from procutil import *
from mpi4py import MPI


#def run_plugin_libcim(name, **kwargs):
#    """Function encoding sequence of PSI module an plugin calls so that
#    plugin_libcim can be called via :py:func:`driver.energy`.
#
#    >>> energy('plugin_libcim')
#
#    """
#    lowername = name.lower()
#    kwargs = kwargs_lower(kwargs)
#
#    # Your plugin's PsiMod run sequence goes here
#    PsiMod.set_global_option('BASIS', 'sto-3g')
#    PsiMod.set_local_option('PLUGIN_LIBCIM', 'PRINT', 1)
#    energy('scf', **kwargs)
#    returnvalue = PsiMod.plugin('plugin_libcim.so')
#
#    return returnvalue


def run_plugin_libcim_parallel(name, **kwargs):
    """Function encoding sequence of PSI module and plugin calls so that
    Eugene's plugin_libcim can be called via :py:func:`driver.energy`.

    >>> energy('cim-ccsd(t)')

    """
    lowername = name.lower()
    kwargs = kwargs_lower(kwargs)

    # create a MPI communicator
    comm = MPI.COMM_WORLD
    # get the rank
    rank = comm.Get_rank()
    # get the number of processes
    nproc = comm.Get_size()

    # override symmetry:
    molecule = PsiMod.get_active_molecule()
    molecule.update_geometry()
    molecule.reset_point_group('c1')
    molecule.fix_orientation(1)
    molecule.update_geometry()

    # what type of cim?
    if (lowername == 'cim-ccsd'):
        PsiMod.set_global_option('compute_triples', False)
    if (lowername == 'cim-ccsd(t)'):
        PsiMod.set_global_option('compute_triples', True)

    # some options are not correct when i load two plugins ... set them here
    PsiMod.set_global_option('r_convergence', 1e-7)
    PsiMod.set_global_option('maxiter', 100)

    energy('scf', **kwargs)
    PsiMod.set_global_option('cim_initialize', True)
    PsiMod.plugin('plugin_libcim.so')

    cluster_ccsd = []
    cluster_ccsdt = []
    cluster_t = []
    for i in range(nproc):
        cluster_ccsd.append(0.0)
        cluster_ccsdt.append(0.0)
        cluster_t.append(0.0)

    built_ccsd = 0.0
    built_t = 0.0
    built_ccsdt = 0.0
    built_energy = 0.0
    escf = PsiMod.get_variable('SCF TOTAL ENERGY')
    PsiMod.set_global_option('cim_initialize', False)
    cim_n = 0
    while cim_n < PsiMod.get_variable('CIM CLUSTERS'):
        # if rank owns this work
        if (rank == cim_n%nproc):
           # run plugin_libcim
           PsiMod.set_global_option('CIM_CLUSTER_NUM', cim_n)
           PsiMod.plugin('plugin_libcim.so')
           # accumulate correlation energies
           cluster_ccsd[rank] += PsiMod.get_variable('CURRENT CLUSTER CCSD CORRELATION ENERGY')
           if (lowername == 'cim-ccsd(t)'):
               cluster_ccsdt[rank] += PsiMod.get_variable('CURRENT CLUSTER CCSD(T) CORRELATION ENERGY')
               cluster_t[rank] += PsiMod.get_variable('CURRENT CLUSTER (T) CORRELATION ENERGY')
        cim_n += 1

    PsiMod.flush_outfile()

    # grab energies from other all procs:
    for i in range(nproc):
        if (i>0):
           if (rank == i):
              comm.send(cluster_ccsd[i], dest=0, tag=i+1)
           if (rank == 0):
              cluster_ccsd[i] = comm.recv(source=i, tag=i+1)
   
           if (rank == i):
              comm.send(cluster_ccsdt[i], dest=0, tag=i+1)
           if (rank == 0):
              cluster_ccsdt[i] = comm.recv(source=i, tag=i+1)
   
           if (rank == i):
              comm.send(cluster_t[i], dest=0, tag=i+1)
           if (rank == 0):
              cluster_t[i] = comm.recv(source=i, tag=i+1)

        built_ccsd += cluster_ccsd[i]
        built_ccsdt += cluster_ccsdt[i]
        built_t += cluster_t[i]

    built_energy = built_ccsd
    if (lowername == 'cim-ccsd(t)'):
       built_energy = built_ccsdt

    PsiMod.set_variable('CURRENT ENERGY', built_energy + escf)
    PsiMod.set_variable('CIM-CCSD CORRELATION ENERGY', built_ccsd)
    PsiMod.set_variable('CIM-CCSD TOTAL ENERGY', built_ccsd + escf)
    if (lowername == 'cim-ccsd(t)'):
        PsiMod.set_variable('CIM-CCSD(T) CORRELATION ENERGY', built_ccsdt)
        PsiMod.set_variable('CIM-CCSD(T) TOTAL ENERGY', built_ccsdt + escf)
        PsiMod.set_variable('CIM-(T) CORRELATION ENERGY', built_t)

    if (rank == 0):
       PsiMod.print_out('\n')
       PsiMod.print_out('        CIM-CCSD correlation energy    %20.12lf\n' % PsiMod.get_variable('CIM-CCSD CORRELATION ENERGY'))
       PsiMod.print_out('      * CIM-CCSD total energy          %20.12lf\n' % PsiMod.get_variable('CIM-CCSD TOTAL ENERGY'))
       PsiMod.print_out('\n')
       if (lowername == 'cim-ccsd(t)'):
          PsiMod.print_out('        CIM-(T) correlation energy     %20.12lf\n' % PsiMod.get_variable('CIM-(T) CORRELATION ENERGY'))
          PsiMod.print_out('        CIM-CCSD(T) correlation energy %20.12lf\n' % PsiMod.get_variable('CIM-CCSD(T) CORRELATION ENERGY'))
          PsiMod.print_out('      * CIM-CCSD(T) total energy       %20.12lf\n' % PsiMod.get_variable('CIM-CCSD(T) TOTAL ENERGY'))
          PsiMod.print_out('\n')

    return built_energy + escf

def run_plugin_libcim(name, **kwargs):
    """Function encoding sequence of PSI module and plugin calls so that
    Eugene's plugin_libcim can be called via :py:func:`driver.energy`.

    >>> energy('cim-ccsd(t)')

    """
    lowername = name.lower()
    kwargs = kwargs_lower(kwargs)

    # override symmetry:
    molecule = PsiMod.get_active_molecule()
    molecule.update_geometry()
    molecule.reset_point_group('c1')
    molecule.fix_orientation(1)
    molecule.update_geometry()

    # what type of cim?
    if (lowername == 'cim-ccsd'):
        PsiMod.set_global_option('compute_triples', False)
    if (lowername == 'cim-ccsd(t)'):
        PsiMod.set_global_option('compute_triples', True)

    # some options are not correct when i load two plugins ... set them here
    PsiMod.set_global_option('r_convergence', 1e-7)
    PsiMod.set_global_option('maxiter', 100)

    energy('scf', **kwargs)
    PsiMod.set_global_option('cim_initialize', True)
    PsiMod.plugin('plugin_libcim.so')

    cluster_ccsd = []
    cluster_ccsdt = []
    cluster_t = []
    built_ccsd = 0.0
    built_t = 0.0
    built_ccsdt = 0.0
    built_energy = 0.0
    escf = PsiMod.get_variable('SCF TOTAL ENERGY')
    PsiMod.set_global_option('cim_initialize', False)
    cim_n = 0
    while cim_n < PsiMod.get_variable('CIM CLUSTERS'):
        # run plugin_libcim:
        PsiMod.set_global_option('CIM_CLUSTER_NUM', cim_n)
        PsiMod.plugin('plugin_libcim.so')

        # accumulate correlation energies
        cluster_ccsd.append(PsiMod.get_variable('CURRENT CLUSTER CCSD CORRELATION ENERGY'))
        built_ccsd += PsiMod.get_variable('CURRENT CLUSTER CCSD CORRELATION ENERGY')
        built_energy = built_ccsd
        if (lowername == 'cim-ccsd(t)'):
            cluster_ccsdt.append(PsiMod.get_variable('CURRENT CLUSTER CCSD(T) CORRELATION ENERGY'))
            cluster_t.append(PsiMod.get_variable('CURRENT CLUSTER (T) CORRELATION ENERGY'))
            built_ccsdt += PsiMod.get_variable('CURRENT CLUSTER CCSD(T) CORRELATION ENERGY')
            built_t += PsiMod.get_variable('CURRENT CLUSTER (T) CORRELATION ENERGY')
            built_energy = built_ccsdt

        cim_n += 1

    PsiMod.set_variable('CURRENT ENERGY', built_energy + escf)
    PsiMod.set_variable('CIM-CCSD CORRELATION ENERGY', built_ccsd)
    PsiMod.set_variable('CIM-CCSD TOTAL ENERGY', built_ccsd + escf)
    PsiMod.print_out('\n')
    PsiMod.print_out('        CIM-CCSD correlation energy    %20.12lf\n' % PsiMod.get_variable('CIM-CCSD CORRELATION ENERGY'))
    PsiMod.print_out('      * CIM-CCSD total energy          %20.12lf\n' % PsiMod.get_variable('CIM-CCSD TOTAL ENERGY'))
    PsiMod.print_out('\n')
    if (lowername == 'cim-ccsd(t)'):
        PsiMod.set_variable('CIM-CCSD(T) CORRELATION ENERGY', built_ccsdt)
        PsiMod.set_variable('CIM-CCSD(T) TOTAL ENERGY', built_ccsdt + escf)
        PsiMod.set_variable('CIM-(T) CORRELATION ENERGY', built_t)
        PsiMod.print_out('        CIM-(T) correlation energy     %20.12lf\n' % PsiMod.get_variable('CIM-(T) CORRELATION ENERGY'))
        PsiMod.print_out('        CIM-CCSD(T) correlation energy %20.12lf\n' % PsiMod.get_variable('CIM-CCSD(T) CORRELATION ENERGY'))
        PsiMod.print_out('      * CIM-CCSD(T) total energy       %20.12lf\n' % PsiMod.get_variable('CIM-CCSD(T) TOTAL ENERGY'))
        PsiMod.print_out('\n')

    return built_energy + escf

# Integration with driver routines
procedures['energy']['cim-ccsd(t)'] = run_plugin_libcim_parallel
procedures['energy']['cim-ccsd'] = run_plugin_libcim_parallel

def exampleFN():
    # Your Python code goes here
    pass