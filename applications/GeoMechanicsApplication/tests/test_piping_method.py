import sys
import os
import math

#sys.path.append(os.path.join('..', '..', '..'))
#sys.path.append(os.path.join('..', 'python_scripts'))

import KratosMultiphysics.KratosUnittest as KratosUnittest
import test_helper

class KratosGeoMechanicsPipingMethodTests(KratosUnittest.TestCase):

    def setUp(self):
        # Code here will be placed BEFORE every test in this TestCase.
        pass

    def tearDown(self):
        # Code here will be placed AFTER every test in this TestCase.
        pass

    def test_self_recursive_solution_step(self):
        test_name = 'test_piping_method'
        file_path = test_helper.get_file_path(os.path.join('.', test_name))
        simulation = test_helper.run_kratos(file_path)
        model_part = simulation._list_of_output_processes[0].model_part

if __name__ == '__main__':
    suites = KratosUnittest.KratosSuites
    smallSuite = suites['small'] # These tests are executed by the continuous integration tool
    smallSuite.addTests(KratosUnittest.TestLoader().loadTestsFromTestCases([KratosGeoMechanicsPipingMethodTests]))
    allSuite = suites['all']
    allSuite.addTests(smallSuite)
    KratosUnittest.runTests(suites)
