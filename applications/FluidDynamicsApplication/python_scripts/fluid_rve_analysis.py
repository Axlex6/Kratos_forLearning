import KratosMultiphysics
import KratosMultiphysics.FluidDynamicsApplication
from KratosMultiphysics.FluidDynamicsApplication.fluid_dynamics_analysis import FluidDynamicsAnalysis

# Importing other libraries
import math

class FluidRVEAnalysis(FluidDynamicsAnalysis):
    def __init__(self, model, project_parameters):

        # input parameters of the analysis
        self.boundary_mp_name = project_parameters["rve_settings"]["boundary_mp_name"].GetString()
        self.averaging_mp_name = project_parameters["rve_settings"]["averaging_mp_name"].GetString()
        self.print_rve_post = project_parameters["rve_settings"]["print_rve_post"].GetBool()   

        self.averaging_volume = -1.0  # it will be computed in initialize
        self.domain_size = project_parameters["solver_settings"]["domain_size"].GetInt()
        if(self.domain_size == 2):
            self.strain_size = 3
        elif self.domain_size == 3:
            self.strain_size = 6
        else:
            err_msg = "Wrong 'domain_size' value {}. Expected 2 or 3.".format(self.domain_size)
            raise ValueError(err_msg)

        # Dictionary containin the jump data for periodicity
        self.strains = {}
        
        self.strains["00"] = project_parameters["rve_settings"]["jump_XX"].GetDouble()
        self.strains["11"] = project_parameters["rve_settings"]["jump_YY"].GetDouble()
        self.strains["01"] = project_parameters["rve_settings"]["jump_XY"].GetDouble()
        self.strains["12"] = project_parameters["rve_settings"]["jump_YZ"].GetDouble() # needs to be included in the 3D case?
        if(self.domain_size == 3):
            self.strains["22"] = project_parameters["rve_settings"]["jump_ZZ"].GetDouble()
            self.strains["02"] = project_parameters["rve_settings"]["jump_XZ"].GetDouble()


        self.populate_search_eps = 1e-4 ##tolerance in finding which conditions belong to the surface (will be multiplied by the lenght of the diagonal)
        self.geometrical_search_tolerance = 1e-4 #tolerance to be used in the search of the condition it falls into

        super().__init__(model, project_parameters)

    # Here populate the submodelparts to be used for periodicity
    def ModifyInitialGeometry(self):
        super().ModifyInitialGeometry()
        boundary_mp = self.model[self.boundary_mp_name]
        averaging_mp = self.model[self.averaging_mp_name]

        # Construct auxiliary modelparts
        self.min_corner, self.max_corner = self._DetectBoundingBox(averaging_mp)
        self._ConstructFaceModelParts(self.min_corner, self.max_corner, boundary_mp)

        self.averaging_volume = 1.0
        for i in range(self.domain_size):
            self.averaging_volume *= self.max_corner[i]-self.min_corner[i]
        KratosMultiphysics.Logger.PrintInfo(self._GetSimulationName(), "RVE undeformed averaging volume = ", self.averaging_volume)

    def ModifyAfterSolverInitialize(self):
        boundary_mp = self.model[self.boundary_mp_name]
        averaging_mp = self.model[self.averaging_mp_name]
        
        # Create and fill "strain matrix"
        strain = KratosMultiphysics.Matrix(self.domain_size, self.domain_size)
        strain.fill(0.0)
        
        for i in range(self.domain_size):
            for j in range(i ,self.domain_size):
                strainIndex = str(i) + str(j)
                
                strain[i, j] = self.strains[strainIndex]
                strain[j, i] = self.strains[strainIndex]
                
        self._ApplyPeriodicity(strain, averaging_mp, boundary_mp)

    def _DetectBoundingBox(self, mp):
        min_corner = KratosMultiphysics.Array3()
        min_corner[0] = 1e20
        min_corner[1] = 1e20
        min_corner[2] = 1e20

        max_corner = KratosMultiphysics.Array3()
        max_corner[0] = -1e20
        max_corner[1] = -1e20
        max_corner[2] = -1e20

        for node in mp.Nodes:
            x = node.X
            min_corner[0] = min(min_corner[0], x)
            max_corner[0] = max(max_corner[0], x)

            y = node.Y
            min_corner[1] = min(min_corner[1], y)
            max_corner[1] = max(max_corner[1], y)

            z = node.Z
            min_corner[2] = min(min_corner[2], z)
            max_corner[2] = max(max_corner[2], z)

        KratosMultiphysics.Logger.PrintInfo(self._GetSimulationName(), "Bounding box detected")
        KratosMultiphysics.Logger.PrintInfo(self._GetSimulationName(), "Min. corner = ", min_corner)
        KratosMultiphysics.Logger.PrintInfo(self._GetSimulationName(), "Max. corner = ", max_corner)

        return min_corner, max_corner

    def __PopulateMp(self, face_name, coordinate, component, eps, mp):
        if mp.NumberOfConditions() == 0:
            raise Exception("Boundary_mp is expected to have conditions and has none")

        mp = mp.GetRootModelPart()

        if not mp.HasSubModelPart(face_name):
            mp.CreateSubModelPart(face_name)
        face_mp = mp.GetSubModelPart(face_name)

        for cond in mp.Conditions:
            xc = cond.GetGeometry().Center()
            if abs(xc[component] - coordinate) < eps:
                face_mp.AddCondition(cond)

        node_ids = set()
        for cond in face_mp.Conditions:
            for node in cond.GetNodes():
                if(not node.Is(KratosMultiphysics.SLAVE)):
                    node_ids.add(node.Id)
                    node.Set(KratosMultiphysics.SLAVE)

        face_mp.AddNodes(list(node_ids))
        return face_mp

    def _ConstructFaceModelParts(self, min_corner, max_corner, mp):
        diag_vect = max_corner - min_corner
        diag_lenght = math.sqrt(diag_vect[0]**2 + diag_vect[1]**2 + diag_vect[2]**2 )
        eps = self.populate_search_eps * diag_lenght

        KratosMultiphysics.VariableUtils().SetFlag(KratosMultiphysics.SLAVE, False, mp.Nodes)
        KratosMultiphysics.VariableUtils().SetFlag(KratosMultiphysics.MASTER, False, mp.Nodes)

        # Populate the slave faces
        self.max_x_face = self.__PopulateMp("max_x_face", max_corner[0], 0, eps, mp)
        self.max_y_face = self.__PopulateMp("max_y_face", max_corner[1], 1, eps, mp)
        if self.domain_size == 3:
            self.max_z_face = self.__PopulateMp("max_z_face", max_corner[2], 2, eps, mp)

        # First populate the master faces (min)
        self.min_x_face = self.__PopulateMp("min_x_face", min_corner[0], 0, eps, mp)
        self.min_y_face = self.__PopulateMp("min_y_face", min_corner[1], 1, eps, mp)
        if self.domain_size == 3:
            self.min_z_face = self.__PopulateMp("min_z_face", min_corner[2], 2, eps, mp)

        if self.min_x_face.NumberOfConditions() == 0:
            raise Exception("min_x_face has 0 conditions")
        if self.min_y_face.NumberOfConditions() == 0:
            raise Exception("min_y_face has 0 conditions")
        if self.domain_size == 3 and self.min_z_face.NumberOfConditions() == 0:
            raise Exception("min_z_face has 0 conditions")
      
    def _ApplyPeriodicity(self, strain, volume_mp, boundary_mp):
        # clear
        for constraint in volume_mp.GetRootModelPart().MasterSlaveConstraints:
            constraint.Set(KratosMultiphysics.TO_ERASE)
        volume_mp.GetRootModelPart().RemoveMasterSlaveConstraintsFromAllLevels(
            KratosMultiphysics.TO_ERASE)

        dx = self.max_corner[0] - self.min_corner[0]
        dy = self.max_corner[1] - self.min_corner[1]
        if self.strain_size == 6:
            dz = self.max_corner[2] - self.min_corner[2]
        
        periodicity_utility = KratosMultiphysics.RVEPeriodicityUtility(self._GetSolver().GetComputingModelPart())
       
        # assign periodicity to faces
        direction_x = KratosMultiphysics.Vector([dx, 0.0, 0.0]) if self.strain_size == 6 else KratosMultiphysics.Vector([dx, 0.0])
        periodicity_utility.AssignPeriodicity(self.min_x_face, self.max_x_face, strain, direction_x, self.geometrical_search_tolerance)
        direction_y = KratosMultiphysics.Vector([0.0, dy, 0.0]) if self.strain_size == 6 else KratosMultiphysics.Vector([0.0, dy])      
        periodicity_utility.AssignPeriodicity(self.min_y_face, self.max_y_face, strain, direction_y, self.geometrical_search_tolerance)
        if self.strain_size == 6:
            direction_z = KratosMultiphysics.Vector([0.0, 0.0, dz])
            periodicity_utility.AssignPeriodicity(self.min_z_face, self.max_z_face, strain, direction_z, self.geometrical_search_tolerance)    
        
        periodicity_utility.Finalize(KratosMultiphysics.VELOCITY)
        