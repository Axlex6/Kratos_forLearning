// ==============================================================================
//  KratosOptimizationApplication
//
//  License:         BSD License
//                   license: OptimizationApplication/license.txt
//
//  Main authors:    Reza Najian Asl, https://github.com/RezaNajian
//
// ==============================================================================

// ------------------------------------------------------------------------------
// System includes
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
// External includes
// ------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
// Project includes
// ------------------------------------------------------------------------------
#include "geometries/triangle_2d_3.h"
#include "geometries/triangle_3d_3.h"
#include "geometries/quadrilateral_2d_4.h"
#include "geometries/tetrahedra_3d_4.h"
#include "geometries/hexahedra_3d_8.h"
#include "geometries/hexahedra_3d_27.h"
#include "optimization_application.h"
#include "optimization_application_variables.h"

// ==============================================================================

namespace Kratos
{
    KratosOptimizationApplication::KratosOptimizationApplication() :
        KratosApplication("OptimizationApplication"),  
        /* ELEMENTS */
        mHelmholtzSurfShape3D3N(0, Element::GeometryType::Pointer(new Triangle3D3<NodeType >(Element::GeometryType::PointsArrayType(3)))),
        mHelmholtzSurfThickness3D3N(0, Element::GeometryType::Pointer(new Triangle3D3<NodeType >(Element::GeometryType::PointsArrayType(3)))),
        mHelmholtzBulkShape3D4N(0, Element::GeometryType::Pointer(new Tetrahedra3D4<Node<3> >(Element::GeometryType::PointsArrayType(4)))),
        mHelmholtzBulkTopology3D4N(0, Element::GeometryType::Pointer(new Tetrahedra3D4<Node<3> >(Element::GeometryType::PointsArrayType(4)))),
        /* CONDITIONS */            
        mHelmholtzSurfShapeCondition3D3N(0, Condition::GeometryType::Pointer(new Triangle3D3<NodeType >(Condition::GeometryType::PointsArrayType(3))))              
    {}

 	void KratosOptimizationApplication::Register()
 	{
        KRATOS_INFO("") << "Initializing KratosOptimizationApplication..." << std::endl;

        // Register variables 

        //strain energy
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_STRAIN_ENERGY_D_X);
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_STRAIN_ENERGY_D_CX);
        KRATOS_REGISTER_VARIABLE(D_STRAIN_ENERGY_D_PT);
        KRATOS_REGISTER_VARIABLE(D_STRAIN_ENERGY_D_CT);       
        KRATOS_REGISTER_VARIABLE(D_STRAIN_ENERGY_D_PD);
        KRATOS_REGISTER_VARIABLE(D_STRAIN_ENERGY_D_CD);    
        
        //mass
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_MASS_D_X);
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_MASS_D_CX);
        KRATOS_REGISTER_VARIABLE(D_MASS_D_PT);
        KRATOS_REGISTER_VARIABLE(D_MASS_D_CT);       
        KRATOS_REGISTER_VARIABLE(D_MASS_D_PD);
        KRATOS_REGISTER_VARIABLE(D_MASS_D_CD);      

        //eigenfrequency
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_EIGEN_FREQ_D_X);
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_EIGEN_FREQ_D_CX);
        KRATOS_REGISTER_VARIABLE(D_EIGEN_FREQ_D_PT);
        KRATOS_REGISTER_VARIABLE(D_EIGEN_FREQ_D_CT);       
        KRATOS_REGISTER_VARIABLE(D_EIGEN_FREQ_D_PD);
        KRATOS_REGISTER_VARIABLE(D_EIGEN_FREQ_D_CD);            

        //local_stress
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_LOCAL_STRESS_D_X);
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_LOCAL_STRESS_D_CX);
        KRATOS_REGISTER_VARIABLE(D_LOCAL_STRESS_D_PT);
        KRATOS_REGISTER_VARIABLE(D_LOCAL_STRESS_D_CT);       
        KRATOS_REGISTER_VARIABLE(D_LOCAL_STRESS_D_PD);
        KRATOS_REGISTER_VARIABLE(D_LOCAL_STRESS_D_CD);                  

        //max_stress
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_MAX_STRESS_D_X);
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_MAX_STRESS_D_CX);
        KRATOS_REGISTER_VARIABLE(D_MAX_STRESS_D_PT);
        KRATOS_REGISTER_VARIABLE(D_MAX_STRESS_D_CT);       
        KRATOS_REGISTER_VARIABLE(D_MAX_STRESS_D_PD);
        KRATOS_REGISTER_VARIABLE(D_MAX_STRESS_D_CD);       

        // shape control
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(CX);
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_CX);  
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS(D_X);   

        // thickness control
        KRATOS_REGISTER_VARIABLE(PT);
        KRATOS_REGISTER_VARIABLE(FT);
        KRATOS_REGISTER_VARIABLE(CT);
        KRATOS_REGISTER_VARIABLE(D_CT);  
        KRATOS_REGISTER_VARIABLE(D_PT);    

        // density control
        KRATOS_REGISTER_VARIABLE(PD);
        KRATOS_REGISTER_VARIABLE(FD);
        KRATOS_REGISTER_VARIABLE(CD);
        KRATOS_REGISTER_VARIABLE(D_CD);  
        KRATOS_REGISTER_VARIABLE(D_PD);               

        // For implicit vertex-morphing with Helmholtz PDE
        KRATOS_REGISTER_VARIABLE( HELMHOLTZ_MASS_MATRIX );
        KRATOS_REGISTER_VARIABLE( HELMHOLTZ_SURF_RADIUS_SHAPE );
        KRATOS_REGISTER_VARIABLE( HELMHOLTZ_BULK_RADIUS_SHAPE );
        KRATOS_REGISTER_VARIABLE( COMPUTE_CONTROL_POINTS_SHAPE );
        KRATOS_REGISTER_VARIABLE( HELMHOLTZ_SURF_POISSON_RATIO_SHAPE );
        KRATOS_REGISTER_VARIABLE( HELMHOLTZ_BULK_POISSON_RATIO_SHAPE );
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS( HELMHOLTZ_VARS_SHAPE);
        KRATOS_REGISTER_3D_VARIABLE_WITH_COMPONENTS( HELMHOLTZ_SOURCE_SHAPE);  

        // For thickness optimization
        KRATOS_REGISTER_VARIABLE(HELMHOLTZ_VAR_THICKNESS);
        KRATOS_REGISTER_VARIABLE(HELMHOLTZ_SOURCE_THICKNESS); 
        KRATOS_REGISTER_VARIABLE(HELMHOLTZ_RADIUS_THICKNESS); 

        // For topology optimization
        KRATOS_REGISTER_VARIABLE(HELMHOLTZ_VAR_DENSITY);
        KRATOS_REGISTER_VARIABLE(HELMHOLTZ_SOURCE_DENSITY);
        KRATOS_REGISTER_VARIABLE(HELMHOLTZ_RADIUS_DENSITY);               

        // Shape optimization elements
        KRATOS_REGISTER_ELEMENT("HelmholtzSurfShape3D3N", mHelmholtzSurfShape3D3N); 
        KRATOS_REGISTER_ELEMENT("HelmholtzBulkShape3D4N", mHelmholtzBulkShape3D4N);

        // Topology optimization elements 
        KRATOS_REGISTER_ELEMENT("HelmholtzBulkTopology3D4N", mHelmholtzBulkTopology3D4N);

        // Thickness optimization elements
        KRATOS_REGISTER_ELEMENT("HelmholtzSurfThickness3D3N", mHelmholtzSurfThickness3D3N);        

        // Shape optimization conditions
        KRATOS_REGISTER_CONDITION("HelmholtzSurfShapeCondition3D3N", mHelmholtzSurfShapeCondition3D3N); 

        // KRATOS_REGISTER_VARIABLE(TEST_MAP);
 	}

}  // namespace Kratos.


