// KRATOS___
//     //   ) )
//    //         ___      ___
//   //  ____  //___) ) //   ) )
//  //    / / //       //   / /
// ((____/ / ((____   ((___/ /  MECHANICS
//
//  License:         geo_mechanics_application/license.txt
//
//  Main authors:    Aron Noordam
//

// Application includes
#define _USE_MATH_DEFINES
#include "custom_elements/steady_state_Pw_piping_element.hpp"
#include <math.h>
namespace Kratos
{


template< unsigned int TDim, unsigned int TNumNodes >
Element::Pointer SteadyStatePwPipingElement<TDim,TNumNodes>::
    Create( IndexType NewId,
            NodesArrayType const& ThisNodes,
            PropertiesType::Pointer pProperties ) const
{
    return Element::Pointer( new SteadyStatePwPipingElement( NewId, this->GetGeometry().Create( ThisNodes ), pProperties ) );
}

//----------------------------------------------------------------------------------------
template< unsigned int TDim, unsigned int TNumNodes >
Element::Pointer SteadyStatePwPipingElement<TDim,TNumNodes>::
    Create(IndexType NewId,
           GeometryType::Pointer pGeom,
           PropertiesType::Pointer pProperties) const
{
    return Element::Pointer( new SteadyStatePwPipingElement( NewId, pGeom, pProperties ) );
}
template< unsigned int TDim, unsigned int TNumNodes >
int SteadyStatePwPipingElement<TDim,TNumNodes>::
    Check( const ProcessInfo& rCurrentProcessInfo ) const
{
    KRATOS_TRY
    int ierr = SteadyStatePwInterfaceElement<TDim, TNumNodes>::Check(rCurrentProcessInfo);
    if (ierr != 0) return ierr;
    // todo check piping parameters
    return ierr;
    KRATOS_CATCH( "" );

}

template< unsigned int TDim, unsigned int TNumNodes >
void SteadyStatePwPipingElement<TDim, TNumNodes>::
Initialize(const ProcessInfo& rCurrentProcessInfo)
{
    KRATOS_TRY
    SteadyStatePwInterfaceElement<TDim, TNumNodes>::Initialize(rCurrentProcessInfo);
    const PropertiesType& Prop = this->GetProperties();

    this->CalculateLength(this->GetGeometry());
    this->SetValue(PIPE_EROSION, false);
    this->SetValue(PIPE_HEIGHT, Prop[MINIMUM_JOINT_WIDTH]);

    this->Set(ACTIVE, false);

    KRATOS_CATCH("");
}


template< >
void SteadyStatePwPipingElement<2, 4>::CalculateLength(const GeometryType& Geom)
{
    KRATOS_TRY
        //this->pipe_length = abs(Geom.GetPoint(1)[0] - Geom.GetPoint(0)[0]);
        this->SetValue(PIPE_ELEMENT_LENGTH, abs(Geom.GetPoint(1)[0] - Geom.GetPoint(0)[0]));
	KRATOS_CATCH("")
}

template< >
void SteadyStatePwPipingElement<3, 6>::CalculateLength(const GeometryType& Geom)
{
    KRATOS_ERROR << " Length of SteadyStatePwPipingElement3D6N element is not implemented" << std::endl;
}
template< >
void SteadyStatePwPipingElement<3, 8>::CalculateLength(const GeometryType& Geom)
{
    KRATOS_ERROR << " Length of SteadyStatePwPipingElement3D8N element is not implemented" << std::endl;
}


//----------------------------------------------------------------------------------------
template< unsigned int TDim, unsigned int TNumNodes >
void SteadyStatePwPipingElement<TDim,TNumNodes>::
    CalculateAll(MatrixType& rLeftHandSideMatrix,
                 VectorType& rRightHandSideVector,
                 const ProcessInfo& CurrentProcessInfo,
                 const bool CalculateStiffnessMatrixFlag,
                 const bool CalculateResidualVectorFlag)
{
    KRATOS_TRY
    //Previous definitions
    const PropertiesType& Prop = this->GetProperties();
    const GeometryType& Geom = this->GetGeometry();
    const GeometryType::IntegrationPointsArrayType& IntegrationPoints = Geom.IntegrationPoints( mThisIntegrationMethod );
    const unsigned int NumGPoints = IntegrationPoints.size();

    //Containers of variables at all integration points
    const Matrix& NContainer = Geom.ShapeFunctionsValues( mThisIntegrationMethod );
    const GeometryType::ShapeFunctionsGradientsType& DN_DeContainer = Geom.ShapeFunctionsLocalGradients( mThisIntegrationMethod );
    GeometryType::JacobiansType JContainer(NumGPoints);
    Geom.Jacobian( JContainer, mThisIntegrationMethod );
    Vector detJContainer(NumGPoints);
    Geom.DeterminantOfJacobian(detJContainer,mThisIntegrationMethod);

    //Element variables
    InterfaceElementVariables Variables;
    
    this->InitializeElementVariables(Variables,
                                     Geom,
                                     Prop,
                                     CurrentProcessInfo);

	
    // VG: TODO
    // Perhaps a new parameter to get join width and not minimum joint width
    //Variables.JointWidth = Prop[MINIMUM_JOINT_WIDTH];
    Variables.JointWidth = this->GetValue(PIPE_HEIGHT);
	
    //Auxiliary variables
    array_1d<double,TDim> RelDispVector;
    SFGradAuxVariables SFGradAuxVars;

    // create general parametes of retention law
    RetentionLaw::Parameters RetentionParameters(Geom, this->GetProperties(), CurrentProcessInfo);

    //Loop over integration points
    for ( unsigned int GPoint = 0; GPoint < NumGPoints; ++GPoint) {
        //Compute Np, StrainVector, JointWidth, GradNpT
        noalias(Variables.Np) = row(NContainer,GPoint);

        this->template 
            CalculateShapeFunctionsGradients< Matrix >(Variables.GradNpT,
                                                       SFGradAuxVars,
                                                       JContainer[GPoint],
                                                       Variables.RotationMatrix,
                                                       DN_DeContainer[GPoint],
                                                       NContainer,
                                                       Variables.JointWidth,
                                                       GPoint);

        //Compute BodyAcceleration and Permeability Matrix
        GeoElementUtilities::
            InterpolateVariableWithComponents<TDim, TNumNodes>(Variables.BodyAcceleration,
                                                                NContainer,
                                                                Variables.VolumeAcceleration,
                                                                GPoint );

        InterfaceElementUtilities::FillPermeabilityMatrix( Variables.LocalPermeabilityMatrix,
                                                           Variables.JointWidth,
                                                           Prop[TRANSVERSAL_PERMEABILITY] );

        CalculateRetentionResponse( Variables,
                                    RetentionParameters,
                                    GPoint );

        //Compute weighting coefficient for integration
        Variables.IntegrationCoefficient = 
            this->CalculateIntegrationCoefficient(IntegrationPoints,
                                                  GPoint,
                                                  detJContainer[GPoint]);

        //Contributions to the left hand side
        if (CalculateStiffnessMatrixFlag) this->CalculateAndAddLHS(rLeftHandSideMatrix, Variables);

        //Contributions to the right hand side
        if (CalculateResidualVectorFlag)  this->CalculateAndAddRHS(rRightHandSideVector, Variables, GPoint);

    }

    KRATOS_CATCH( "" )
}

template< >
double SteadyStatePwPipingElement<2, 4>::CalculateWaterPressureGradient(const PropertiesType& Prop, const GeometryType& Geom, double dx)
{
	return abs(Geom[1].FastGetSolutionStepValue(WATER_PRESSURE) - Geom[0].FastGetSolutionStepValue(WATER_PRESSURE)) / dx;
}
template< >
double SteadyStatePwPipingElement<3, 6>::CalculateWaterPressureGradient(const PropertiesType& Prop, const GeometryType& Geom, double dx)
{
    KRATOS_ERROR << " pressure gradient calculation of SteadyStatePwPipingElement3D6N element is not implemented" << std::endl;
    return 0;
}
template< >
double SteadyStatePwPipingElement<3, 8>::CalculateWaterPressureGradient(const PropertiesType& Prop, const GeometryType& Geom, double dx)
{
    KRATOS_ERROR << " pressure gradient calculation of SteadyStatePwPipingElement3D8N element is not implemented" << std::endl;
    return 0;
}

/// <summary>
///  Calculate the particle diameter for the particles in the pipe. The particle diameter equals d70, 
/// when the unmodified sellmeijer piping rule is used. 
/// </summary>
/// <param name="Prop"></param>
/// <param name="Geom"></param>
/// <returns></returns>
template< unsigned int TDim, unsigned int TNumNodes >
double SteadyStatePwPipingElement<TDim, TNumNodes>::CalculateParticleDiameter(const PropertiesType& Prop)
{
    double diameter;

    if (Prop[PIPE_MODIFIED_D])
        diameter = 2.08e-4 * pow((Prop[PIPE_D_70] / 2.08e-4), 0.4);
    else
        diameter = Prop[PIPE_D_70];
    return diameter;
}


/// <summary>
/// Calculates the equilibrium pipe height of a piping element according to Sellmeijers rule
/// </summary>
/// <param name="Prop"></param>
/// <param name="Geom"></param>
/// <returns></returns>
template< unsigned int TDim, unsigned int TNumNodes >
double SteadyStatePwPipingElement<TDim,TNumNodes>:: CalculateEquilibriumPipeHeight(const PropertiesType& Prop, const GeometryType& Geom, double pipe_length)
{
    const double modelFactor = Prop[PIPE_MODEL_FACTOR];
    const double eta = Prop[PIPE_ETA];
    const double theta = Prop[PIPE_THETA];
    const double SolidDensity = Prop[DENSITY_SOLID];
    const double FluidDensity = Prop[DENSITY_WATER];
 
    // calculate pressure gradient over element
    double dpdx = CalculateWaterPressureGradient(Prop, Geom, pipe_length);

    // calculate particle diameter
    double particle_d = CalculateParticleDiameter(Prop);
    
    // todo calculate slope of pipe, currently pipe is assumed to be horizontal
    const double pipeSlope = 0;

    // return infinite when dpdx is 0
    if (dpdx < DBL_EPSILON)
    { 
        return 1e10;
    }

    // gravity is taken from first node
    array_1d<double, 3> gravity_array= Geom[0].FastGetSolutionStepValue(VOLUME_ACCELERATION);
    const double gravity = norm_2(gravity_array);
	
    return modelFactor * M_PI / 3.0 * particle_d * (SolidDensity - FluidDensity) * gravity * eta  * sin((theta  + pipeSlope) * M_PI / 180.0) / cos(theta * M_PI / 180.0) / dpdx;

}

template< unsigned int TDim, unsigned int TNumNodes >
bool SteadyStatePwPipingElement<TDim, TNumNodes>:: InEquilibrium(const PropertiesType& Prop, const GeometryType& Geom)
{
	// Calculation if Element in Equilibrium
    //double pipeEquilibriumPipeHeight = CalculateEquilibriumPipeHeight(Prop, Geom);

	// Logic if in equilibrium
	
    const bool inEquilibrium = false;
	return inEquilibrium;

}
	
template class SteadyStatePwPipingElement<2,4>;
template class SteadyStatePwPipingElement<3,6>;
template class SteadyStatePwPipingElement<3,8>;

} // Namespace Kratos
