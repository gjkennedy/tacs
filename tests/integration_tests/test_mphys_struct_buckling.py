import os
import unittest

import numpy as np
import openmdao.api as om
from mphys.core import Multipoint, MPhysVariables
from mphys.scenarios import ScenarioStructural

import tacs.mphys
from openmdao_analysis_base_test import OpenMDAOTestCase
from tacs import elements, constitutive, TACS

"""
This is a simple 1m by 2m plate made up of four quad shell elements.
The plate is structurally loaded under a compression load and a unit force,
"f_aero_struct", is applied on on every node. The mass and KSFailure of the plate
are evaluated as outputs and have their partial and total sensitivities checked.
"""

base_dir = os.path.dirname(os.path.abspath(__file__))
bdf_file = os.path.join(base_dir, "./input_files/debug_plate.bdf")

# Historical reference values for function outputs
FUNC_REFS = {
    "analysis.eigsb_0": -1.08864541,
    "analysis.eigsb_1": 1.08938765,
}

# Inputs to check total sensitivities wrt
wrt = [
    f"mesh.fea_mesh.{MPhysVariables.Structures.Mesh.COORDINATES}",
    "dv_struct",
    MPhysVariables.Structures.Loads.AERODYNAMIC,
]


class ProblemTest(OpenMDAOTestCase.OpenMDAOTest):
    N_PROCS = 1  # this is how many MPI processes to use for this TestCase.

    def setup_problem(self, dtype):
        """
        Setup openmdao problem object we will be testing.
        """

        # Overwrite default tolerances
        if dtype == complex:
            self.rtol = 1e-7
            self.dh = 1e-50
        else:
            self.rtol = 1e-3
            self.dh = 1e-6

        # Callback function used to setup TACS element objects and DVs
        def element_callback(
            dv_num, comp_id, comp_descript, elem_descripts, special_dvs, **kwargs
        ):
            rho = 2780.0  # density, kg/m^3
            E = 73.1e9  # elastic modulus, Pa
            nu = 0.33  # poisson's ratio
            ys = 324.0e6  # yield stress, Pa
            thickness = 0.012
            min_thickness = 0.002
            max_thickness = 0.05

            # Setup (isotropic) property and constitutive objects
            prop = constitutive.MaterialProperties(rho=rho, E=E, nu=nu, ys=ys)
            # Set one thickness dv for every component
            con = constitutive.IsoShellConstitutive(
                prop, t=thickness, tNum=dv_num, tlb=min_thickness, tub=max_thickness
            )

            # For each element type in this component,
            # pass back the appropriate tacs element object
            transform = None
            elem = elements.Quad4Shell(transform, con)

            return elem

        def problem_setup(scenario_name, fea_assembler, problem):
            """
            Helper function to add fixed forces and eval functions
            to structural problems used in tacs builder
            """
            # Set convergence to be tight for test
            problem.setOption("L2Convergence", 1e-20)
            problem.setOption("L2ConvergenceRel", 1e-20)

        def buckling_setup(scenario_name, fea_assembler):
            """
            Helper function to add fixed forces and eval functions
            to structural problems used in tacs builder
            """
            bucklingOptions = {"writeSolution": False}
            problem = fea_assembler.createBucklingProblem(
                "buckling", sigma=1e0, numEigs=2, options=bucklingOptions
            )
            problem.setOption("L2Convergence", 1e-20)
            problem.setOption("L2ConvergenceRel", 1e-20)
            return problem

        class Top(Multipoint):
            def setup(self):
                tacs_builder = tacs.mphys.TacsBuilder(
                    mesh_file=bdf_file,
                    element_callback=element_callback,
                    problem_setup=problem_setup,
                    buckling_setup=buckling_setup,
                    check_partials=True,
                    coupling_loads=[MPhysVariables.Structures.Loads.AERODYNAMIC],
                    write_solution=False,
                )
                tacs_builder.initialize(self.comm)
                ndv_struct = tacs_builder.get_ndv()

                dvs = self.add_subsystem("dvs", om.IndepVarComp(), promotes=["*"])
                dvs.add_output("dv_struct", np.array(ndv_struct * [0.01]))

                f_size = tacs_builder.get_ndof() * tacs_builder.get_number_of_nodes()
                forces = self.add_subsystem("forces", om.IndepVarComp(), promotes=["*"])
                f = np.zeros(f_size)
                f[1::3] = -1e0
                forces.add_output(
                    MPhysVariables.Structures.Loads.AERODYNAMIC,
                    val=np.ones(f_size),
                    distributed=True,
                )

                self.add_subsystem("mesh", tacs_builder.get_mesh_coordinate_subsystem())
                self.mphys_add_scenario(
                    "analysis", ScenarioStructural(struct_builder=tacs_builder)
                )
                self.connect(
                    f"mesh.{MPhysVariables.Structures.Mesh.COORDINATES}",
                    f"analysis.{MPhysVariables.Structures.COORDINATES}",
                )
                self.connect("dv_struct", "analysis.dv_struct")
                self.connect(
                    MPhysVariables.Structures.Loads.AERODYNAMIC,
                    f"analysis.{MPhysVariables.Structures.Loads.AERODYNAMIC}",
                )

        prob = om.Problem()
        prob.model = Top()

        return prob

    def setup_funcs(self):
        """
        Create a dict of functions to be tested and a list of inputs
        to test their sensitivities with respect to.
        """
        return FUNC_REFS, wrt

    # This test is very difficult to verify with FD, so we only run it w/ CS
    @unittest.skipIf(TACS.dtype != complex, "Skipping test in real mode.")
    def test_partials(self):
        """
        Test partial sensitivities using fd/cs
        """
        return OpenMDAOTestCase.OpenMDAOTest.test_partials(self)

    # This test is very difficult to verify with FD, so we only run it w/ CS
    @unittest.skipIf(TACS.dtype != complex, "Skipping test in real mode.")
    def test_totals(self):
        """
        Test partial sensitivities using fd/cs
        """
        return OpenMDAOTestCase.OpenMDAOTest.test_totals(self)
