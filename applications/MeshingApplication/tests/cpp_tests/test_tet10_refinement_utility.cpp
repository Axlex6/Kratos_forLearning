//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:     BSD License
//           Kratos default license: kratos/license.txt
//
//  Main authors:    Ariadna Cortés
//
//

// Project includes
#include "custom_utilities/tet10_refinement_utility.h"
#include "geometries/triangle_3d_3.h"



namespace Kratos {
namespace Testing {

    //Type definitions 
    typedef Node<3> NodeType;
    typedef Node<3>::Pointer NodePtrType;
    typedef Geometry<NodeType> GeometryType;
    typedef GeometryType::Pointer GeometryPtrType;
    typedef GeometryType::GeometriesArrayType GeometryArrayType;
    typedef GeometryType::PointsArrayType PointsArrayType;

    //Auxiliar functions
    /**
     * @brief Returns the distance between two 3D points.
     * @param rPoint0 reference to the first point
     * @param rPoint1 reference an array of 3 coordinates representing the second point
     * @return Distance 
     */  
    static double Distance(const NodeType& Point0, const array_1d<double,3>& Point1) {
        const double lx = Point0.X() - Point1[0];
        const double ly = Point0.Y() - Point1[1];
        const double lz = Point0.Z() - Point1[2];

        const double length = lx * lx + ly * ly + lz * lz;

        return std::sqrt( length );
    }

    KRATOS_TEST_CASE_IN_SUITE(Tet10RefinementUtility, KratosMeshingApplicationFastSuite)
    {
        Model MyModel;
        ModelPart& modelpart = MyModel.CreateModelPart("Tetrahedras"); 
        modelpart.CreateNewNode(1, 1, 0, 0); 
        modelpart.CreateNewNode(2, 0, 1, 0);
        modelpart.CreateNewNode(3, 0, 0, 1);
        modelpart.CreateNewNode(4, 0, 0, 2);
        modelpart.CreateNewNode(5, 0, 2, 0);
        Properties::Pointer p_properties_1(new Properties(0)); 
        Element::Pointer tetra1 = modelpart.CreateNewElement("Element3D4N", 1, {1, 2, 3, 4}, p_properties_1);
        Element::Pointer tetra2 = modelpart.CreateNewElement("Element3D4N", 2, {2, 3, 4, 5}, p_properties_1);

        Condition::Pointer cond1;
        cond1 = modelpart.CreateNewCondition("SurfaceCondition3D3N", 3, {1, 2, 3}, p_properties_1);

        Tet10RefinementUtility refineTetra(modelpart); 
        refineTetra.LocalRefineTet10Mesh(true);

        KRATOS_CHECK_EQUAL(modelpart.Nodes().size(),14); //There are 14 nodes (10 for each tetra but 6 are shared) 
        KRATOS_CHECK_EQUAL(modelpart.Elements().size(),2); //No new elements are added
        KRATOS_CHECK_EQUAL(modelpart.Conditions().size(),1); //No new conditions are added
        
        for(auto elem : modelpart.Elements()) {
            KRATOS_CHECK_EQUAL(typeid(elem.GetGeometry()), typeid(Tetrahedra3D10<NodeType>));
            GeometryPtrType geom = elem.pGetGeometry();

            auto points = geom->Points();
            KRATOS_CHECK_EQUAL(Distance(points[0],points[1]), Distance(points[0],points[4]) + Distance(points[4],points[1]) );
            KRATOS_CHECK_EQUAL(Distance(points[1],points[2]), Distance(points[1],points[5]) + Distance(points[5],points[2]) );
            KRATOS_CHECK_EQUAL(Distance(points[2],points[3]), Distance(points[2],points[9]) + Distance(points[9],points[3]) );
            KRATOS_CHECK_EQUAL(Distance(points[3],points[0]), Distance(points[3],points[7]) + Distance(points[7],points[0]) );
            KRATOS_CHECK_EQUAL(Distance(points[0],points[2]), Distance(points[0],points[6]) + Distance(points[6],points[0]) );
            KRATOS_CHECK_EQUAL(Distance(points[1],points[3]), Distance(points[1],points[8]) + Distance(points[8],points[1]) );
        }
        for(auto cond : modelpart.Conditions()) {
            KRATOS_CHECK_EQUAL(typeid(cond.GetGeometry()), typeid(Triangle3D6<NodeType>));
            GeometryPtrType geom = cond.pGetGeometry();

            auto points = geom->Points();
            KRATOS_CHECK_EQUAL(Distance(points[0],points[1]), Distance(points[0],points[3]) + Distance(points[3],points[1]) );
            KRATOS_CHECK_EQUAL(Distance(points[1],points[2]), Distance(points[1],points[4]) + Distance(points[4],points[2]) );
            KRATOS_CHECK_EQUAL(Distance(points[2],points[0]), Distance(points[2],points[5]) + Distance(points[5],points[0]) );
        }

    }

}  // namespace Testing.
}  // namespace Kratos.