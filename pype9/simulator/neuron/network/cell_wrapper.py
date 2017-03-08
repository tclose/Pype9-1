"""
  Author: Thomas G. Close (tclose@oist.jp)
  Copyright: 2012-2014 Thomas G. Close.
  License: This file is part of the "NineLine" package, which is released under
           the MIT Licence, see LICENSE for details.
"""
from __future__ import absolute_import
try:
    from mpi4py import MPI  # @UnresolvedImport @UnusedImport
except:
    pass
import pyNN.models
from pype9.simulator.neuron.cells import CellMetaClass
import logging
from pype9.simulator.base.network.cell_wrapper import (
    PyNNCellWrapper as BasePyNNCellWrapper,
    PyNNCellWrapperMetaClass as BasePyNNCellWrapperMetaClass)
from ..units import UnitHandler

logger = logging.getLogger("PyNN")


class PyNNCellWrapper(BasePyNNCellWrapper, pyNN.models.BaseCellType):

    """
    Extends the vanilla Cell to include all the PyNN requirements
    """
    UnitHandler = UnitHandler


class PyNNCellWrapperMetaClass(BasePyNNCellWrapperMetaClass):

    loaded_celltypes = {}
    UnitHandler = UnitHandler

    def __new__(cls, name, component_class, default_properties,
                initial_state, initial_regime, build_mode='lazy', silent=False,
                solver_name=None, standalone=False, **kwargs):  # @UnusedVariable @IgnorePep8
        try:
            celltype = cls.loaded_celltypes[
                (component_class.name, component_class.url)]
        except KeyError:
            model = CellMetaClass(component_class=component_class,
                                  default_properties=default_properties,
                                  initial_state=initial_state, name=name,
                                  build_mode=build_mode, silent=silent,
                                  solver_name=solver_name,
                                  standalone=False, **kwargs)
            dct = {'model': model,
                   'default_properties': default_properties,
                   'initial_state': initial_state,
                   'initial_regime': initial_regime}
            celltype = super(PyNNCellWrapperMetaClass, cls).__new__(
                cls, name, (PyNNCellWrapper,), dct)
            assert set(celltype.recordable) == set(
                model().recordable.keys()), \
                ("Mismatch of recordable keys between CellPyNN ('{}') and "
                 "Cell class '{}' ('{}')".format(
                     "', '".join(sorted(celltype.recordable)), name,
                     "', '".join(sorted(model().recordable.keys()))))
            # If the url where the celltype is defined is specified save the
            # celltype to be retried later
            if component_class.url is not None:
                cls.loaded_celltypes[(name, component_class.url)] = celltype
        return celltype