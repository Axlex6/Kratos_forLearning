import KratosMultiphysics

import KratosMultiphysics.GeoMechanicsApplication as GMA
import KratosMultiphysics.KratosUnittest as KratosUnittest

class TestSetMultipleMovingLoadsProcess(KratosUnittest.TestCase):

    def setUp(self):
        self.current_model = KratosMultiphysics.Model()
        self.rmp = self.current_model.CreateModelPart("root_part")
        self.mp = self.rmp.CreateSubModelPart("solid_part")
        self.cmp = self.rmp.CreateSubModelPart("compute_part")

    def tearDown(self):
        self.rmp.Clear();

    def checkRHS(self, rhs, expected_res):
        """
        routine to check calculation of rhs side within context of testing SetMovingLoad
        Returns
        -------

        """
        self.assertAlmostEqual(rhs[0], expected_res[0])
        self.assertAlmostEqual(rhs[1], expected_res[1])
        self.assertAlmostEqual(rhs[2], expected_res[2])
        self.assertAlmostEqual(rhs[3], expected_res[3])

    def _TestSetMultipleMovingLoads(self):
        """
        Tests a moving load on 1 condition element, where the nodes of the element are sorted in the direction of the
        moving load
        Returns
        -------

        """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        cond = self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -2.0, 0.0, 0.0])

        # move load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -1.5, 0.0, -0.5])

    def _TestSetMultipleMovingLoadsConfigurationPositive(self):
        """
        Tests a moving load on 1 condition element, where the nodes of the element are sorted in the direction of the
        moving load, including a positive configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0],
                    "configuration"             : [0.25]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -1.5, 0.0, -0.5])

        # move load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -1.0, 0.0, -1.0])

    def _TestSetMultipleMovingLoadsConfigurationNegative(self):
        """
        Tests a moving load on 1 condition element, where the nodes of the element are sorted in the direction of the
        moving load, , including a negative configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0],
                    "configuration"             : [-0.25]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, 0.0])

        # move load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -2.0, 0.0, 0.0])

    def _TestSetMultipleMovingLoadsConfigurationCombined(self):
        """
        Tests a moving load on 1 condition element, where the nodes of the element are sorted in the direction of the
        moving load, , including a combined configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0],
                    "configuration"             : [-0.25, 0, 0.25]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        conditions = []
        conditions.append(self.cmp.GetCondition(2))
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()


        # set load on node
        all_rhs = []
        for cond in conditions:
            # initialise matrices
            lhs = KratosMultiphysics.Matrix(0, 0)
            rhs = KratosMultiphysics.Vector(0)
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))


        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -2.0, 0.0, 0.0])
        self.checkRHS(all_rhs[2], [0.0, -1.5, 0.0, -0.5])

        # move load within first element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -2.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.5, 0.0, -0.5])
        self.checkRHS(all_rhs[2], [0.0, -1.0, 0.0, -1.0])

    def _TestSetMultipleMovingLoadsReverseGeom(self):
        """
        Tests a moving load on 1 condition element, where the nodes of the element are reversed compared to the
        direction of the moving load
        Returns
        -------

        """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0], second_coord[1], 0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [2, 1], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                   {
                       "help"                       : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                       "model_part_name"            : "solid_part",
                       "compute_model_part_name"    : "compute_part",
                       "variable_name"              : "POINT_LOAD",
                       "load"                       : [0.0, -2.0, 0.0],
                       "direction"                  : [1,1,1],
                       "velocity"                   : 1,
                       "origin"                     : [0,0,0]
                   }
                   """
                                                   )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME,
                                0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME,
                                0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        # cond.SetValue(SMA.MOVING_LOAD_LOCAL_DISTANCE, 0)
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, -2.0])

        # move load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -0.5, 0.0, -1.5])

    def _TestSetMultipleMovingLoadsReverseGeomConfigurationPositive(self):
        """
        Tests a moving load on 1 condition element, where the nodes of the element are reversed compared to the
        direction of the moving load, including a positive configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0], second_coord[1], 0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [2, 1], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                   {
                       "help"                       : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                       "model_part_name"            : "solid_part",
                       "compute_model_part_name"    : "compute_part",
                       "variable_name"              : "POINT_LOAD",
                       "load"                       : [0.0, -2.0, 0.0],
                       "direction"                  : [1,1,1],
                       "velocity"                   : 1,
                       "origin"                     : [0,0,0],
                       "configuration"              : [0.25]
                   }
                   """
                                                   )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME,
                                0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME,
                                0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -0.5, 0.0, -1.5])

        # move load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -1.0, 0.0, -1.0])

    def _TestSetMultipleMovingLoadsReverseGeomConfigurationNegative(self):
        """
        Tests a moving load on 1 condition element, where the nodes of the element are reversed compared to the
        direction of the moving load, including a negative configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0], second_coord[1], 0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [2, 1], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                   {
                       "help"                       : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                       "model_part_name"            : "solid_part",
                       "compute_model_part_name"    : "compute_part",
                       "variable_name"              : "POINT_LOAD",
                       "load"                       : [0.0, -2.0, 0.0],
                       "direction"                  : [1,1,1],
                       "velocity"                   : 1,
                       "origin"                     : [0,0,0],
                       "configuration"              : [-0.25]
                   }
                   """
                                                   )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME,
                                0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME,
                                0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)
        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        # cond.SetValue(SMA.MOVING_LOAD_LOCAL_DISTANCE, 0)
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, 0.0])

        # move load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, -2.0])

    def _TestSetMultipleMovingLoadsMultipleConditions(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load
        Returns
        -------

        """

        #create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 3], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME,
                                                  0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME,
                                                  0.5)
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -2.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load within first element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -1.0, 0.0, -1.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to element connection element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, -2.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to next element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.0, 0.0, -1.0])

    def _TestSetMultipleMovingLoadsMultipleConditionsOffSetPositive(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load
        Returns
        -------

        """

        #create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 3], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0],
                    "configuration"             : [0.5]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME,
                                                  0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME,
                                                  0.5)
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -1.0, 0.0, -1.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load within first element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, -2.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to element connection element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.0, 0.0, -1.0])

        # move load to next element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, -2.0])

    def _TestSetMultipleMovingLoadsMultipleConditionsOffSetNegative(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load
        Returns
        -------

        """

        #create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 3], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0],
                    "configuration"             : [-0.5]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME,
                                                  0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME,
                                                  0.5)
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load within first element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -2.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to element connection element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -1.0, 0.0, -1.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to next element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, -2.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to next element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.0, 0.0, -1.0])

    def _TestSetMultipleMovingLoadsMultipleConditionsReversed(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is reversed compared to the moving
        direction of the load
        Returns
        -------

        """

        # create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [3, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 1], self.mp.GetProperties()[1])

        # set parameters and process info
        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.5)
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, -2.0])

        # move load within first element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.0, 0.0, -1.0])

        # move load to element connection element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, -2.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to next element, also increase time step
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.75)
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -1.5, 0.0, -0.5])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

    def _TestSetMultipleMovingLoadsMultipleConditionsReversedConfigurationPositive(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is reversed compared to the moving
        direction of the load, including a positive configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [3, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 1], self.mp.GetProperties()[1])

        # set parameters and process info
        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0],
                    "configuration"             : [0.5]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.5)
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.0, 0.0, -1.0])

        # move load within first element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, -2.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to element connection element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -1.0, 0.0, -1.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load to next element, also increase time step
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.75)
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

    def _TestSetMultipleMovingLoadsMultipleConditionsReversedConfigurationNegative(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is reversed compared to the moving
        direction of the load, including a negative configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [3, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 1], self.mp.GetProperties()[1])

        # set parameters and process info
        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0,0,0],
                    "configuration"             : [-0.5]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.5)
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

        # move load within first element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, -2.0])

        # move load to element connection element
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.0, 0.0, -1.0])

        # move load to next element, also increase time step
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.75)
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # calculate load
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -0.5, 0.0, -1.5])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

    def _TestSetMultipleMovingLoadsMultipleConditionsDifferentOrigin(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load
        Returns
        -------

        """

        #create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 3], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [1.25,0,0]
                }
                """
                                                         )
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, -1.5, 0.0, -0.5])


    def _TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginConfigurationPositive(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load, including a positive configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        #create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 3], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [1.25,0,0],
                    "configuration"             : [0.75]
                }
                """
                                                         )
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, -2.0])


    def _TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginConfigurationNegative(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load, including a negative configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        #create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 3], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [1.25,0.0,0.0],
                    "configuration"             : [-0.25]
                }
                """
                                                         )
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, -2.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])


    def _TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginReversed(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load
        Returns
        -------

        """

        # create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [3, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 1], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [1.25,0,0]
                }
                """
                                                         )
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -0.5, 0.0, -1.5])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])


    def _TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginReversedConfigurationPositive(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load, including a positive configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [3, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 1], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [1.25,0,0],
                    "configuration"             : [0.25]
                }
                """
                                                         )
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, -1.0, 0.0, -1.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, 0.0])

    def _TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginReversedConfigurationNegative(self):
        """
        Tests a moving load on 2 condition elements, where the order of the elements is sorted in the direction of the
        moving load, including a negative configuration along line condition direction in velocity direction.
        Returns
        -------

        """

        # create nodes
        second_coord = [1.0, 0.0, 0.0]
        third_coord = [2.0, 0.0, 0.0]
        self.mp.CreateNewNode(1, 0.0, 0.0, 0.0)
        self.mp.CreateNewNode(2, second_coord[0],second_coord[1],second_coord[2])
        self.mp.CreateNewNode(3, third_coord[0], third_coord[1], third_coord[2])

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [3, 2], self.mp.GetProperties()[1])
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 2, [2, 1], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [1.25,0,0],
                    "configuration"             : [-1.25]
                }
                """
                                                         )
        process = GMA.SetMultipleMovingLoadsProcess(self.mp,parameters)

        # get conditions
        conditions = []
        conditions.append(self.cmp.GetCondition(3))
        conditions.append(self.cmp.GetCondition(4))

        # initialize and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0,0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        all_rhs = []
        for cond in conditions:
            cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)
            all_rhs.append(list(rhs))

        self.checkRHS(all_rhs[0], [0.0, 0.0, 0.0, 0.0])
        self.checkRHS(all_rhs[1], [0.0, 0.0, 0.0, -2.0])

    def _TestSetMultipleMovingLoadsWithLoadFunction(self):
        """
       Tests a moving load on a condition element, where the load is a function of time
       Returns
       -------

       """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : ["0.0", "-2.0*t", "0.0"],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0.5,0,0]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)

        # get conditions
        cond = self.cmp.GetCondition(2)


        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, 0.0])

        # change time and recalculate load
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0.5)
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -0.5, 0.0, -0.5])

    def _TestSetMultipleMovingLoadsWithLoadFunctionConfigurationPositive(self):
        """
       Tests a moving load on a condition element, where the load is a function of time, including a positive configuration along line condition direction in velocity direction.
       Returns
       -------

       """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : ["0.0", "-2.0*t", "0.0"],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0.5,0,0],
                    "configuration"             : [0.25]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, 0.0])

        # change time and recalculate load
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0.5)
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -0.25, 0.0, -0.75])

    def _TestSetMultipleMovingLoadsWithLoadFunctionConfigurationNegative(self):
        """
       Tests a moving load on a condition element, where the load is a function of time, including a negative configuration along line condition direction in velocity direction.
       Returns
       -------

       """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : ["0.0", "-2.0*t", "0.0"],
                    "direction"                 : [1,1,1],
                    "velocity"                  : 1,
                    "origin"                    : [0.5,0,0],
                    "configuration"             : [-0.25]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, 0.0])

        # change time and recalculate load
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0.5)
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -0.75, 0.0, -0.25])

    def _TestSetMultipleMovingLoadsWithVelocityFunction(self):
        """
       Tests a moving load on a condition element, where the load velocity is a function of time.

       Returns
       -------

       """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : "2.0*t",
                    "origin"                    : [0.0,0,0]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -2.0, 0.0, 0.0])

        # change time and recalculate load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -2.0, 0.0, 0.0])

        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0.5)

        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -1.5, 0.0, -0.5])

    def _TestSetMultipleMovingLoadsWithVelocityFunctionConfigurationPositive(self):
        """
       Tests a moving load on a condition element, where the load velocity is a function of time, including a positive configuration along line condition direction in velocity direction.

       Returns
       -------

       """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                      : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"           : "solid_part",
                    "compute_model_part_name"   : "compute_part",
                    "variable_name"             : "POINT_LOAD",
                    "load"                      : [0.0, -2.0, 0.0],
                    "direction"                 : [1,1,1],
                    "velocity"                  : "2.0*t",
                    "origin"                    : [0.0,0,0],
                    "configuration"             : [0.5]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -1.0, 0.0, -1.0])

        # change time and recalculate load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -1.0, 0.0, -1.0])

        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0.5)

        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -0.5, 0.0, -1.5])

    def _TestSetMultipleMovingLoadsWithVelocityFunctionConfigurationNegative(self):
        """
       Tests a moving load on a condition element, where the load velocity is a function of time, including a negative configuration along line condition direction in velocity direction.

       Returns
       -------

       """

        # create nodes
        second_coord = [1, 0, 0.0]
        self.mp.CreateNewNode(1,0.0,0.0,0.0)
        self.mp.CreateNewNode(2,second_coord[0],second_coord[1],0.0)

        # create condition
        self.mp.CreateNewCondition("MovingLoadCondition2D2N", 1, [1,2], self.mp.GetProperties()[1])

        parameters = KratosMultiphysics.Parameters("""
                {
                    "help"                    : "This process applies multiple moving load conditions belonging to a modelpart. The load moves over line elements.",
                    "model_part_name"         : "solid_part",
                    "compute_model_part_name" : "compute_part",
                    "variable_name"           : "POINT_LOAD",
                    "load"                    : [0.0, -2.0, 0.0],
                    "direction"               : [1,1,1],
                    "velocity"                : "2.0*t",
                    "origin"                  : [0.0,0,0],
                    "configuration"           : [-0.25]
                }
                """
                                                         )
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0)
        self.mp.ProcessInfo.SetValue(KratosMultiphysics.DELTA_TIME, 0.25)

        process = GMA.SetMultipleMovingLoadsProcess(self.mp, parameters)
        cond = self.cmp.GetCondition(2)

        # initialise and set load
        process.ExecuteInitialize()
        process.ExecuteInitializeSolutionStep()

        # initialise matrices
        lhs = KratosMultiphysics.Matrix(0, 0)
        rhs = KratosMultiphysics.Vector(0)

        # set load on node
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, 0.0])

        # change time and recalculate load
        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, 0.0, 0.0, 0.0])

        self.mp.ProcessInfo.SetValue(KratosMultiphysics.TIME, 0.5)

        process.ExecuteFinalizeSolutionStep()
        process.ExecuteInitializeSolutionStep()

        # check if interpolation is done correctly
        cond.CalculateLocalSystem(lhs, rhs, self.mp.ProcessInfo)

        self.checkRHS(rhs, [0.0, -2.0, 0.0, 0.0])


    def test_SetMultipleMovingLoads(self):
        self._TestSetMultipleMovingLoads()

    def test_SetMultipleMovingLoadsConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsConfigurationPositive()

    def test_SetMultipleMovingLoadsConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsConfigurationNegative()

    def test_SetMultipleMovingLoadsConfigurationCombined(self):
        self._TestSetMultipleMovingLoadsConfigurationCombined()

    def test_SetMultipleMovingLoadsReverseGeom(self):
        self._TestSetMultipleMovingLoadsReverseGeom()

    def test_SetMultipleMovingLoadsReverseGeomConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsReverseGeomConfigurationPositive()

    def test_SetMultipleMovingLoadsReverseGeomConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsReverseGeomConfigurationNegative()

    def test_SetMultipleMovingLoadsMultipleConditions(self):
        self._TestSetMultipleMovingLoadsMultipleConditions()

    def test_SetMultipleMovingLoadsMultipleConditionsConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsOffSetPositive()

    def test_SetMultipleMovingLoadsMultipleConditionsConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsOffSetNegative()

    def test_SetMultipleMovingLoadsMultipleConditionsReversed(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsReversed()

    def test_SetMultipleMovingLoadsMultipleConditionsReversedConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsReversedConfigurationPositive()

    def test_SetMultipleMovingLoadsMultipleConditionsReversedConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsReversedConfigurationNegative()

    def test_SetMultipleMovingLoadsMultipleConditionsDifferentOrigin(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsDifferentOrigin()

    def test_SetMultipleMovingLoadsMultipleConditionsDifferentOriginConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginConfigurationPositive()

    def test_SetMultipleMovingLoadsMultipleConditionsDifferentOriginConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginConfigurationNegative()

    def test_SetMultipleMovingLoadsMultipleConditionsDifferentOriginReversed(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginReversed()

    def test_SetMultipleMovingLoadsMultipleConditionsDifferentOriginReversedConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginReversedConfigurationPositive()

    def test_SetMultipleMovingLoadsMultipleConditionsDifferentOriginReversedConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsMultipleConditionsDifferentOriginReversedConfigurationNegative()

    def test_SetMultipleMovingLoadsWithLoadFunction(self):
        self._TestSetMultipleMovingLoadsWithLoadFunction()

    def test_SetMultipleMovingLoadsWithLoadFunctionConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsWithLoadFunctionConfigurationPositive()

    def test_SetMultipleMovingLoadsWithLoadFunctionConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsWithLoadFunctionConfigurationNegative()

    def test_SetMultipleMovingLoadsWithVelocityFunction(self):
        self._TestSetMultipleMovingLoadsWithVelocityFunction()

    def test_SetMultipleMovingLoadsWithVelocityFunctionConfigurationPositive(self):
        self._TestSetMultipleMovingLoadsWithVelocityFunctionConfigurationPositive()

    def test_SetMultipleMovingLoadsWithVelocityFunctionConfigurationNegative(self):
        self._TestSetMultipleMovingLoadsWithVelocityFunctionConfigurationNegative()


if __name__ == '__main__':
    KratosUnittest.main()
