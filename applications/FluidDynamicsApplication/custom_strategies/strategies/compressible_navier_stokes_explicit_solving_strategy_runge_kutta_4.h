//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Ruben Zorrilla
//
//

#if !defined(KRATOS_COMPRESSIBLE_NAVIER_STOKES_EXPLICIT_SOLVING_STRATEGY_RUNGE_KUTTA_4)
#define KRATOS_COMPRESSIBLE_NAVIER_STOKES_EXPLICIT_SOLVING_STRATEGY_RUNGE_KUTTA_4

// System includes
#include <functional>

// External includes

// Project includes
#include "includes/define.h"
#include "includes/model_part.h"
#include "processes/find_nodal_h_process.h"
#include "processes/compute_nodal_gradient_process.h"
#include "solving_strategies/strategies/explicit_solving_strategy_runge_kutta_4.h"
#include "utilities/element_size_calculator.h"

// Application includes
#include "fluid_dynamics_application_variables.h"
#include "custom_processes/shock_detection_process.h"

namespace Kratos
{

///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

///@}
///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{

/** @brief Explicit solving strategy base class
 * @details This is the base class from which we will derive all the explicit strategies (FE, RK4, ...)
 */
template <class TSparseSpace, class TDenseSpace>
class CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4 : public ExplicitSolvingStrategyRungeKutta4<TSparseSpace, TDenseSpace>
{
public:
    ///@name Type Definitions
    ///@{

    /// The base class definition
    typedef ExplicitSolvingStrategyRungeKutta4<TSparseSpace, TDenseSpace> BaseType;

    /// The explicit builder and solver definition
    typedef typename BaseType::ExplicitBuilderType ExplicitBuilderType;

    /// The local vector definition
    typedef typename TDenseSpace::VectorType LocalSystemVectorType;

    /// Pointer definition of CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4
    KRATOS_CLASS_POINTER_DEFINITION(CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4);

    /// Local Flags
    KRATOS_DEFINE_LOCAL_FLAG(SHOCK_CAPTURING);

    ///@}
    ///@name Life Cycle
    ///@{

    /**
     * @brief Default constructor. (with parameters)
     * @param rModelPart The model part of the problem
     * @param ThisParameters The configuration parameters
     */
    explicit CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4(
        ModelPart &rModelPart,
        Parameters ThisParameters)
        : BaseType(rModelPart)
    {
        // Validate and assign defaults
        ThisParameters = this->ValidateAndAssignParameters(ThisParameters, this->GetDefaultParameters());
        this->AssignSettings(ThisParameters);
    }

    /**
     * @brief Default constructor.
     * @param rModelPart The model part to be computed
     * @param pExplicitBuilder The pointer to the explicit builder and solver
     * @param MoveMeshFlag The flag to set if the mesh is moved or not
     */
    explicit CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4(
        ModelPart &rModelPart,
        typename ExplicitBuilderType::Pointer pExplicitBuilder,
        bool MoveMeshFlag = false,
        int RebuildLevel = 0)
        : BaseType(rModelPart, pExplicitBuilder, MoveMeshFlag, RebuildLevel)
    {
    }

    /**
     * @brief Default constructor.
     * @param rModelPart The model part to be computed
     * @param MoveMeshFlag The flag to set if the mesh is moved or not
     */
    explicit CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4(
        ModelPart &rModelPart,
        bool MoveMeshFlag = false,
        int RebuildLevel = 0)
        : BaseType(rModelPart, MoveMeshFlag, RebuildLevel)
    {
    }

    /** Copy constructor.
     */
    CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4(const CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4 &Other) = delete;

    /** Destructor.
     */
    virtual ~CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4() = default;

    ///@}
    ///@name Operators
    ///@{


    ///@}
    ///@name Operations
    ///@{

    /**
     * @brief This method provides the defaults parameters to avoid conflicts between the different constructors
     * @return The default parameters
     */
    Parameters GetDefaultParameters() const override
    {
        Parameters default_parameters = Parameters(R"(
        {
            "name" : "compressible_navier_stokes_explicit_explicit_solving_strategy_runge_kutta_4",
            "rebuild_level" : 0,
            "move_mesh_flag": false,
            "shock_capturing" : true,
            "nithiarasu_smoothing" : false
        })");

        // Getting base class default parameters
        const Parameters base_default_parameters = BaseType::GetDefaultParameters();
        default_parameters.RecursivelyAddMissingParameters(base_default_parameters);
        return default_parameters;
    }

    /**
     * @brief Returns the name of the class as used in the settings (snake_case format)
     * @return The name of the class
     */
    static std::string Name()
    {
        return "compressible_navier_stokes_explicit_solving_strategy_runge_kutta_4";
    }

    /**
     * @brief This method assigns settings to member variables
     * @param ThisParameters Parameters that are assigned to the member variables
     */
    void AssignSettings(const Parameters ThisParameters) override
    {
        // Base class assign settings call
        BaseType::AssignSettings(ThisParameters);

        // Set the specific compressible NS settings
        mShockCapturing = ThisParameters["shock_capturing"].GetBool();
        mNithiarasuSmoothing = ThisParameters["nithiarasu_smoothing"].GetBool();
    }

    /**
     * @brief Initialization of member variables and prior operations
     * In this method we call the base strategy initialize and initialize the time derivatives
     * This is required to prevent OpenMP errors as the time derivatives are stored in the non-historical database
     */
    void Initialize() override
    {
        auto& r_model_part = BaseType::GetModelPart();
        const auto& r_process_info = r_model_part.GetProcessInfo();
        const unsigned int dim = r_process_info[DOMAIN_SIZE];

        // Call the base RK4 finalize substep method
        BaseType::Initialize();

        // Initialize the non-historical database values
        for (auto& r_node : r_model_part.GetCommunicator().LocalMesh().Nodes()) {
            // Initialize the unknowns time derivatives to zero
            r_node.SetValue(DENSITY_TIME_DERIVATIVE, 0.0);
            r_node.SetValue(MOMENTUM_TIME_DERIVATIVE, MOMENTUM_TIME_DERIVATIVE.Zero());
            r_node.SetValue(TOTAL_ENERGY_TIME_DERIVATIVE, 0.0);
            // Initialize the shock capturing magnitudes
            // r_node.SetValue(SHOCK_SENSOR, 0.0);
            // r_node.SetValue(SHOCK_CAPTURING_VISCOSITY, 0.0);
            // r_node.SetValue(SHOCK_CAPTURING_CONDUCTIVITY, 0.0);
            // r_node.SetValue(VELOCITY, VELOCITY.Zero());
            // r_node.SetValue(CHARACTERISTIC_VELOCITY, CHARACTERISTIC_VELOCITY.Zero());
            r_node.SetValue(SMOOTHED_DENSITY, 0.0);
            r_node.SetValue(SMOOTHED_TOTAL_ENERGY, 0.0);
            r_node.SetValue(SMOOTHED_MOMENTUM, SMOOTHED_MOMENTUM.Zero());
        }

        // If required, initialize the OSS projection variables
        if (r_process_info[OSS_SWITCH]) {
            for (auto& r_node : r_model_part.GetCommunicator().LocalMesh().Nodes()) {
                r_node.SetValue(NODAL_AREA, 0.0);
                r_node.SetValue(DENSITY_PROJECTION, 0.0);
                r_node.SetValue(TOTAL_ENERGY_PROJECTION, 0.0);
                r_node.SetValue(MOMENTUM_PROJECTION, ZeroVector(3));
            }
        }

        // If required, initialize the orthogonal projection shock capturing variables
        if (mShockCapturing) {
            // Initialize nodal values
            for (auto& r_node : r_model_part.GetCommunicator().LocalMesh().Nodes()) {
                r_node.SetValue(NODAL_AREA, 0.0);
                r_node.SetValue(MOMENTUM_GRADIENT, ZeroMatrix(dim,dim));
                r_node.SetValue(PRESSURE_GRADIENT, ZeroVector(3));
                r_node.SetValue(TOTAL_ENERGY_GRADIENT, ZeroVector(3));
                r_node.SetValue(DENSITY_GRADIENT, ZeroVector(3));
            }

            // Initialize elemental values
            for (auto& r_elem : r_model_part.GetCommunicator().LocalMesh().Elements()) {
                r_elem.SetValue(SHOCK_CAPTURING_VISCOSITY, 0.0);
                r_elem.SetValue(SHOCK_CAPTURING_CONDUCTIVITY, 0.0);
                r_elem.SetValue(MOMENTUM_GRADIENT, ZeroMatrix(dim,dim));
                r_elem.SetValue(PRESSURE_GRADIENT, ZeroVector(3));
                r_elem.SetValue(TOTAL_ENERGY_GRADIENT, ZeroVector(3));
                r_elem.SetValue(DENSITY_GRADIENT, ZeroVector(3));
            }
        }

        // // Requirements in case the shock capturing is needed
        // if (mShockCapturing) {
        //     InitializeShockCapturing();
        // }
    }

    /**
     * @brief Initialize the Runge-Kutta step
     * In case the mesh has been updated in the previous step we need to reinitialize the shock capturing
     * This includes the calculation of the nodal element size, nodal area and nodal neighbours
     */
    void InitializeSolutionStep() override
    {
        // Call the base RK4 initialize substep method
        BaseType::InitializeSolutionStep();

//         // Initialize acceleration variables
//         auto &r_model_part = BaseType::GetModelPart();
//         const int n_nodes = r_model_part.NumberOfNodes();
// #pragma omp parallel for
//         for (int i_node = 0; i_node < n_nodes; ++i_node) {
//             auto it_node = r_model_part.NodesBegin() + i_node;
//             it_node->GetValue(DENSITY_TIME_DERIVATIVE) = 0.0;
//             it_node->GetValue(MOMENTUM_TIME_DERIVATIVE) = ZeroVector(3);
//             it_node->GetValue(TOTAL_ENERGY_TIME_DERIVATIVE) = 0.0;
//         }

        // Calculate the magnitudes time derivatives
        UpdateUnknownsTimeDerivatives(1.0);

        // const auto& r_model_part = BaseType::GetModelPart();
        // const auto& r_process_info = r_model_part.GetProcessInfo();
        // if (r_process_info[OSS_SWITCH]) {
        //     CalculateOrthogonalSubScalesProjection();
        // }

        // Perform orthogonal projection shock capturing
        if (mShockCapturing) {
            CalculateOrthogonalProjectionShockCapturing();
        }

        // // If the mesh has changed, reinitialize the shock capturing method
        // if (BaseType::MoveMeshFlag() && mShockCapturing) {
        //     InitializeShockCapturing();
        // }
    }

    /**
     * @brief Finalize the Runge-Kutta step
     * In this method we calculate the final linearised time derivatives after the final update
     * These will be the time derivatives employed in the first RK4 sub step of the next time step
     */
    void FinalizeSolutionStep() override
    {
        // Call the base RK4 finalize substep method
        BaseType::FinalizeSolutionStep();

        // // Calculate the magnitudes time derivatives with the obtained solution
        // UpdateUnknownsTimeDerivatives(1.0);

        // Apply the momentum slip condition
        if (mApplySlipCondition) {
            ApplySlipCondition();
        }

        // Do the values smoothing
        if (mNithiarasuSmoothing) {
            CalculateValuesSmoothing();
        }
    }

    /// Turn back information as a string.
    std::string Info() const override
    {
        return "CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4";
    }

    /// Print information about this object.
    void PrintInfo(std::ostream &rOStream) const override
    {
        rOStream << Info();
    }

    /// Print object's data.
    void PrintData(std::ostream &rOStream) const override
    {
        rOStream << Info();
    }

    ///@}

protected:
    ///@name Protected static Member Variables
    ///@{


    ///@}
    ///@name Protected member Variables
    ///@{


    ///@}
    ///@name Protected Operators
    ///@{


    ///@}
    ///@name Protected Operations
    ///@{

    void SolveWithLumpedMassMatrix() override
    {
        // Call the base RK4 strategy to do the explicit update
        BaseType::SolveWithLumpedMassMatrix();

        // // If proceeds, do the intermediate PAD
        // if (mPAD) {
        //     // CLIPPING TEST
        //     // TODO: ADD OPENMP PRAGMAS
        //     const double aux_eps = 1.0e-6;
        //     for (auto& r_node : BaseType::GetModelPart().Nodes()) {
        //         double& r_rho = r_node.FastGetSolutionStepValue(DENSITY);
        //         auto& r_mom = r_node.FastGetSolutionStepValue(MOMENTUM);
        //         double& r_enr = r_node.FastGetSolutionStepValue(TOTAL_ENERGY);

        //         // Check that the density is above the minimum limit
        //         if (!r_node.IsFixed(DENSITY)) {
        //             if (r_rho < mMinDensity) {
        //                 // Update the residual vector accordingly
        //                 r_node.SetValue(DENSITY_PAD, true);
        //                 r_rho = mMinDensity;
        //             }
        //         }

        //         // Check that the energy is above the minimum limit
        //         if (!r_node.IsFixed(TOTAL_ENERGY)) {
        //             const double enr_lower_bound = inner_prod(r_mom, r_mom) / r_rho + aux_eps;
        //             if (r_enr < enr_lower_bound) {
        //                 r_enr = enr_lower_bound;
        //             }
        //         }
        //     }
        // }
    }

    void InitializeRungeKuttaIntermediateSubStep() override
    {
        // Call the base RK4 to perform the initialize intermediate RK sub step
        BaseType::InitializeRungeKuttaIntermediateSubStep();

        // Approximate the unknowns time derivatives with a FE scheme
        // These will be used in the next RK substep residual calculation to compute the subscales
        auto& r_model_part = BaseType::GetModelPart();
        auto& r_process_info = r_model_part.GetProcessInfo();
        // const unsigned int rk_sub_step = r_process_info.GetValue(RUNGE_KUTTA_STEP);
        // if (rk_sub_step > 1) {
        //     const double sub_step_acc_coeff = 0.5;
        //     UpdateUnknownsTimeDerivatives(sub_step_acc_coeff);
        // }

        // Calculate the Orthogonal SubsScales projections
        if (r_process_info[OSS_SWITCH]) {
            CalculateOrthogonalSubScalesProjection();
        }

        // Perform orthogonal projection shock capturing
        // if (mShockCapturing) {
        //     CalculateOrthogonalProjectionShockCapturing();
        // }

        // // Perform shock capturing
        // if (mShockCapturing) {
        //     // Call the shock detection process
        //     mpShockDetectionProcess->ExecuteInitializeSolutionStep();
        //     // Add the corresponding artificial magnitudes
        //     // CalculateShockCapturingMagnitudes();
        // }
    }


    void FinalizeRungeKuttaLastSubStep() override
    {
        // Call the base RK4 finalize substep method
        BaseType::FinalizeRungeKuttaLastSubStep();

        // Apply the momentum slip condition
        //TODO: THIS SHOULDN'T BE REQUIRED --> DOING IT AFTER THE FINAL UPDATE MUST BE ENOUGH
        if (mApplySlipCondition) {
            ApplySlipCondition();
        }
    }

    void InitializeRungeKuttaLastSubStep() override
    {
        // Call the base RK4 to perform the initialize intermediate RK sub step
        BaseType::InitializeRungeKuttaLastSubStep();

        // // Approximate the unknowns time derivatives with a FE scheme
        // // These will be used in the next RK substep residual calculation to compute the subscales
        // const double sub_step_acc_coeff = 1.0;
        // UpdateUnknownsTimeDerivatives(sub_step_acc_coeff);

        // Calculate the Orthogonal SubsScales projections
        auto& r_model_part = BaseType::GetModelPart();
        const auto& r_process_info = r_model_part.GetProcessInfo();
        if (r_process_info[OSS_SWITCH]) {
            CalculateOrthogonalSubScalesProjection();
        }

        // // Perform orthogonal projection shock capturing
        // if (mShockCapturing) {
        //     CalculateOrthogonalProjectionShockCapturing();
        // }

        // // Perform shock capturing
        // if (mShockCapturing) {
        //     // Call the shock detection process
        //     mpShockDetectionProcess->ExecuteInitializeSolutionStep();
        //     // Add the corresponding artificial magnitudes
        //     CalculateShockCapturingMagnitudes();
        // }
    }

    /**
     * @brief Finalize the Runge-Kutta intermediate substep
     * In this method we calculate the linearised time derivatives after the intemediate substep
     */
    void FinalizeRungeKuttaIntermediateSubStep() override
    {
        // Call the base RK4 finalize substep method
        BaseType::FinalizeRungeKuttaIntermediateSubStep();

        // Apply the momentum slip condition
        if (mApplySlipCondition) {
            ApplySlipCondition();
        }
    }

    ///@}
    ///@name Protected  Access
    ///@{


    ///@}
    ///@name Protected Inquiry
    ///@{


    ///@}
    ///@name Protected LifeCycle
    ///@{


    ///@}
private:
    ///@name Static Member Variables
    ///@{


    ///@}
    ///@name Member Variables
    ///@{

    bool mShockCapturing = true;
    bool mApplySlipCondition = true;
    bool mNithiarasuSmoothing = false;
    const bool mPAD = false;
    const double mMinDensity = 1.0e-2;
    const double mMinTotalEnergy = 1.0e-6;

    ShockDetectionProcess::UniquePointer mpShockDetectionProcess = nullptr;

    ///@}
    ///@name Private Operators
    ///@{


    ///@}
    ///@name Private Operations
    ///@{

    /**
     * @brief Update the compressible Navier-Stokes unknowns time derivatives
     * This method approximates the compressible Navier-Stokes unknowns time derivatives
     * These are required to calculate the inertial stabilization terms in the compressible NS element
     * To that purpose a linear Forward-Euler interpolation is used
     */
    void UpdateUnknownsTimeDerivatives(const double SubStepAccCoefficient)
    {
        const double dt = BaseType::GetDeltaTime();
        KRATOS_ERROR_IF(dt < 1.0e-12) << "ProcessInfo DELTA_TIME is close to zero." << std::endl;
        auto &r_model_part = BaseType::GetModelPart();

#pragma omp parallel for
        for (int i_node = 0; i_node < static_cast<int>(r_model_part.NumberOfNodes()); ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;

            // Density DOF time derivative
            const double& r_rho = it_node->FastGetSolutionStepValue(DENSITY);
            const double& r_rho_old = it_node->FastGetSolutionStepValue(DENSITY, 1);
            it_node->GetValue(DENSITY_TIME_DERIVATIVE) = SubStepAccCoefficient * (r_rho - r_rho_old) / dt;

            // Momentum DOF time derivative
            const auto& r_mom = it_node->FastGetSolutionStepValue(MOMENTUM);
            const auto& r_mom_old = it_node->FastGetSolutionStepValue(MOMENTUM, 1);
            it_node->GetValue(MOMENTUM_TIME_DERIVATIVE) = SubStepAccCoefficient * (r_mom - r_mom_old) / dt;

            // Total energy DOF time derivative
            const double& r_tot_enr = it_node->FastGetSolutionStepValue(TOTAL_ENERGY);
            const double& r_tot_enr_old = it_node->FastGetSolutionStepValue(TOTAL_ENERGY, 1);
            it_node->GetValue(TOTAL_ENERGY_TIME_DERIVATIVE) = SubStepAccCoefficient * (r_tot_enr - r_tot_enr_old) / dt;
        }
    }

    void CalculateOrthogonalSubScalesProjection()
    {
        // Get model part data
        auto& r_model_part = BaseType::GetModelPart();
        const int n_nodes = r_model_part.NumberOfNodes();
        const int n_elem = r_model_part.NumberOfElements();
        const auto& r_process_info = r_model_part.GetProcessInfo();
        const unsigned int block_size = r_process_info[DOMAIN_SIZE] + 2;

        // Get the required data from the explicit builder and solver
        const auto p_explicit_bs = BaseType::pGetExplicitBuilder();
        const auto& r_lumped_mass_vector = p_explicit_bs->GetLumpedMassMatrixVector();

        // Initialize the projection values
#pragma omp parallel for
        for (int i_node = 0; i_node < n_nodes; ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;
            it_node->GetValue(NODAL_AREA) = 0.0;
            it_node->GetValue(DENSITY_PROJECTION) = 0.0;
            it_node->GetValue(MOMENTUM_PROJECTION) = ZeroVector(3);
            it_node->GetValue(TOTAL_ENERGY_PROJECTION) = 0.0;
        }

        // Calculate the residuals projection
        double dens_proj;
        double tot_ener_proj;
        array_1d<double,3> mom_proj;
#pragma omp parallel for
        for (int i_elem = 0; i_elem < n_elem; ++i_elem) {
            auto it_elem = r_model_part.ElementsBegin() + i_elem;
            // Calculate the projection values
            it_elem->Calculate(DENSITY_PROJECTION, dens_proj, r_process_info);
            it_elem->Calculate(MOMENTUM_PROJECTION, mom_proj, r_process_info);
            it_elem->Calculate(TOTAL_ENERGY_PROJECTION, tot_ener_proj, r_process_info);
            // Calculate the NODAL_AREA
            // TODO: This is not probably required each time
            auto& r_geom = it_elem->GetGeometry();
            const unsigned int n_nodes = r_geom.PointsNumber();
            const double geom_domain_size = r_geom.DomainSize();
            const double aux_weight = geom_domain_size / static_cast<double>(n_nodes);
            for (auto& r_node : r_geom) {
#pragma omp atomic
                r_node.GetValue(NODAL_AREA) += aux_weight;
            }
        }

#pragma omp parallel for
        for (int i_node = 0; i_node < n_nodes; ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;
            const double nodal_area = it_node->GetValue(NODAL_AREA);
            it_node->GetValue(DENSITY_PROJECTION) /= nodal_area;
            it_node->GetValue(MOMENTUM_PROJECTION) /= nodal_area;
            it_node->GetValue(TOTAL_ENERGY_PROJECTION) /= nodal_area;
        }
    }

    void CalculateOrthogonalProjectionShockCapturing()
    {
        // Calculate the model part data
        auto& r_model_part = BaseType::GetModelPart();
        const int n_nodes = r_model_part.NumberOfNodes();
        const int n_elems = r_model_part.NumberOfElements();
        const int dim = r_model_part.GetProcessInfo()[DOMAIN_SIZE];
        // const unsigned int block_size = dim + 2;

        // // Get the required data from the explicit builder and solver
        // const auto p_explicit_bs = BaseType::pGetExplicitBuilder();
        // const auto& r_lumped_mass_vector = p_explicit_bs->GetLumpedMassMatrixVector();

        // Initialize the values to zero
#pragma omp parallel for
        for (int i_node = 0; i_node < n_nodes; ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;
            it_node->GetValue(NODAL_AREA) = 0.0;
            it_node->GetValue(DENSITY_GRADIENT) = ZeroVector(3);
            it_node->GetValue(PRESSURE_GRADIENT) = ZeroVector(3);
            it_node->GetValue(MOMENTUM_GRADIENT) = ZeroMatrix(dim,dim);
            it_node->GetValue(TOTAL_ENERGY_GRADIENT) = ZeroVector(3);
        }

        // Set the functor to calculate the element size
        // Note that this assumes a unique geometry in the computational mesh
        std::function<double(Geometry<Node<3>>& rGeometry)> avg_h_function;
        const GeometryData::KratosGeometryType geometry_type = (r_model_part.ElementsBegin()->GetGeometry()).GetGeometryType();
        switch (geometry_type) {
            case GeometryData::KratosGeometryType::Kratos_Triangle2D3:
                avg_h_function = [&](Geometry<Node<3>>& rGeometry){return ElementSizeCalculator<2,3>::AverageElementSize(rGeometry);};
                break;
            case GeometryData::KratosGeometryType::Kratos_Tetrahedra3D4:
                avg_h_function = [&](Geometry<Node<3>>& rGeometry){return ElementSizeCalculator<3,4>::AverageElementSize(rGeometry);};
                break;
            default:
                KRATOS_ERROR << "Asking for a non-implemented geometry.";
        }

        // Loop the elements to project the gradients
        // Note that it is assumed that the gradient is constant within the element
        // Hence, only one Gauss point is used
        Geometry<Node<3>>::ShapeFunctionsGradientsType dNdX_container;
#pragma omp parallel for private(dNdX_container)
        for (int i_elem = 0; i_elem < n_elems; ++i_elem) {
            auto it_elem = r_model_part.ElementsBegin() + i_elem;
            auto& r_geom = it_elem->GetGeometry();
            const unsigned int n_nodes = r_geom.PointsNumber();
            const double geom_domain_size = r_geom.DomainSize();
            const double aux_weight = geom_domain_size / static_cast<double>(n_nodes);

            // Get fluid properties
            const auto p_prop = it_elem->pGetProperties();
            const double heat_capacity_ratio = p_prop->GetValue(HEAT_CAPACITY_RATIO);

            // Calculate the gradients in the center of the element
            auto& r_elem_mom_grad = it_elem->GetValue(MOMENTUM_GRADIENT);
            auto& r_elem_rho_grad = it_elem->GetValue(DENSITY_GRADIENT);
            auto& r_elem_pres_grad = it_elem->GetValue(PRESSURE_GRADIENT);
            auto& r_elem_tot_ener_grad = it_elem->GetValue(TOTAL_ENERGY_GRADIENT);
            r_elem_mom_grad = ZeroMatrix(dim, dim);
            r_elem_pres_grad = ZeroVector(3);
            r_elem_rho_grad = ZeroVector(3);
            r_elem_tot_ener_grad = ZeroVector(3);
            dNdX_container = r_geom.ShapeFunctionsIntegrationPointsGradients(dNdX_container, GeometryData::GI_GAUSS_1);
            const auto& r_dNdX = dNdX_container[0];

            for (unsigned int i_node = 0; i_node < n_nodes; ++i_node) {
                auto& r_node = r_geom[i_node];
                const auto node_dNdX = row(r_dNdX, i_node);
                const auto& r_rho = r_node.FastGetSolutionStepValue(DENSITY);
                const auto& r_mom = r_node.FastGetSolutionStepValue(MOMENTUM);
                const double& r_tot_ener = r_node.FastGetSolutionStepValue(TOTAL_ENERGY);
                const double r_mom_norm_squared = r_mom[0] * r_mom[0] + r_mom[1] * r_mom[1] + r_mom[2] * r_mom[2];
                const double i_node_p = (heat_capacity_ratio - 1.0) * (r_tot_ener - 0.5 * r_mom_norm_squared / r_rho);
                for (int d1 = 0; d1 < dim; ++d1) {
                    r_elem_rho_grad[d1] += node_dNdX(d1) * r_rho;
                    r_elem_pres_grad[d1] += node_dNdX(d1) * i_node_p;
                    r_elem_tot_ener_grad[d1] += node_dNdX(d1) * r_tot_ener;
                    for (int d2 = 0; d2 < dim; ++d2) {
                        r_elem_mom_grad(d1,d2) += node_dNdX(d1) * r_mom[d2];
                    }
                }
            }

            // Project the computed gradients to the nodes
            for (unsigned int i_node = 0; i_node < n_nodes; ++i_node) {
                // Get nodal values
                auto& r_node = r_geom[i_node];
                auto& r_node_mom_grad = r_node.GetValue(MOMENTUM_GRADIENT);
                auto& r_node_pres_grad = r_node.GetValue(PRESSURE_GRADIENT);
                auto& r_node_rho_grad = r_node.GetValue(DENSITY_GRADIENT);
                auto& r_node_tot_ener_grad = r_node.GetValue(TOTAL_ENERGY_GRADIENT);
                for (int d1 = 0; d1 < dim; ++d1) {
#pragma omp atomic
                    r_node_rho_grad[d1] += aux_weight * r_elem_rho_grad[d1];
#pragma omp atomic
                    r_node_pres_grad[d1] += aux_weight * r_elem_pres_grad[d1];
#pragma omp atomic
                    r_node_tot_ener_grad[d1] += aux_weight * r_elem_tot_ener_grad[d1];
                    for (int d2 = 0; d2 < dim; ++d2) {
#pragma omp atomic
                        r_node_mom_grad(d1,d2) += aux_weight * r_elem_mom_grad(d1,d2);
                    }
                }
#pragma omp atomic
                r_node.GetValue(NODAL_AREA) += aux_weight;
            }
        }

#pragma omp parallel for
        for (int i_node = 0; i_node < n_nodes; ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;
            const double weight = it_node->GetValue(NODAL_AREA);
            it_node->GetValue(DENSITY_GRADIENT) /= weight;
            it_node->GetValue(PRESSURE_GRADIENT) /= weight;
            it_node->GetValue(MOMENTUM_GRADIENT) /= weight;
            it_node->GetValue(TOTAL_ENERGY_GRADIENT) /= weight;
        }

        // Calculate shock capturing values
        const double zero_tol = 1.0e-12;

        double midpoint_rho;
        double midpoint_pres;
        double midpoint_tot_ener;
        array_1d<double,3> midpoint_v, midpoint_m;
        Matrix midpoint_mom_grad_proj;
        Vector midpoint_rho_grad_proj;
        Vector midpoint_pres_grad_proj;
        Vector midpoint_tot_ener_grad_proj;
        // Vector N_container, midpoint_tot_ener_grad_proj;
// #pragma omp parallel for private(N_container, midpoint_v, midpoint_mom_grad_proj, midpoint_tot_ener_grad_proj)
#pragma omp parallel for private(midpoint_v, midpoint_m, midpoint_rho, midpoint_mom_grad_proj, midpoint_tot_ener_grad_proj, midpoint_pres_grad_proj, midpoint_rho_grad_proj)
        for (int i_elem = 0; i_elem < n_elems; ++i_elem) {
            auto it_elem = r_model_part.ElementsBegin() + i_elem;
            auto& r_geom = it_elem->GetGeometry();
            const unsigned int n_nodes = r_geom.PointsNumber();

            // Get fluid properties
            const auto p_prop = it_elem->pGetProperties();
            const double heat_capacity_ratio = p_prop->GetValue(HEAT_CAPACITY_RATIO);

            // Interpolate the nodal projection values in the midpoint and calculate the average velocity norm
            midpoint_rho = 0.0;
            midpoint_pres = 0.0;
            midpoint_tot_ener = 0.0;
            midpoint_v = ZeroVector(3);
            midpoint_m = ZeroVector(3);
            midpoint_mom_grad_proj = ZeroMatrix(dim, dim);
            midpoint_rho_grad_proj = ZeroVector(3);
            midpoint_pres_grad_proj = ZeroVector(3);
            midpoint_tot_ener_grad_proj = ZeroVector(3);
            const double midpoint_N = 1.0 / static_cast<double>(n_nodes);
            for (unsigned int i_node = 0; i_node < n_nodes; ++i_node) {
                const auto& r_node = r_geom[i_node];
                // Interpolate the nodal projection values in the midpoint
                const auto& r_node_rho_grad = r_node.GetValue(DENSITY_GRADIENT);
                const auto& r_node_pres_grad = r_node.GetValue(PRESSURE_GRADIENT);
                const auto& r_node_mom_grad = r_node.GetValue(MOMENTUM_GRADIENT);
                const auto& r_node_tot_ener_grad = r_node.GetValue(TOTAL_ENERGY_GRADIENT);
                midpoint_mom_grad_proj += midpoint_N * r_node_mom_grad;
                midpoint_rho_grad_proj += midpoint_N * r_node_rho_grad;
                midpoint_pres_grad_proj += midpoint_N * r_node_pres_grad;
                midpoint_tot_ener_grad_proj += midpoint_N * r_node_tot_ener_grad;
                // Calculate the midpoint velocity
                const auto& r_mom = r_node.FastGetSolutionStepValue(MOMENTUM);
                const double& r_rho = r_node.FastGetSolutionStepValue(DENSITY);
                midpoint_v += midpoint_N * r_mom / r_rho;
                // Calculate the midpoint momentum
                midpoint_m += midpoint_N * r_mom;
                // Calculate the midpoint total energy
                const double& r_tot_ener = r_node.FastGetSolutionStepValue(TOTAL_ENERGY);
                midpoint_tot_ener += midpoint_N * r_tot_ener;
                // Calculate the midpoint pressure
                const double r_mom_norm_squared = r_mom[0] * r_mom[0] + r_mom[1] * r_mom[1] + r_mom[2] * r_mom[2];
                const double i_node_p = (heat_capacity_ratio - 1.0) * (r_tot_ener - 0.5 * r_mom_norm_squared / r_rho);
                midpoint_pres += midpoint_N * i_node_p;
                // Calculate the midpoint density
                midpoint_rho += midpoint_N * r_rho;
            }

            // Calculate the norms of the gradients
            // Total energy gradients
            const auto& r_tot_ener_elem_grad = it_elem->GetValue(TOTAL_ENERGY_GRADIENT);
            const double tot_ener_grad_norm = norm_2(r_tot_ener_elem_grad);
            const double tot_ener_grad_proj_norm = norm_2(midpoint_tot_ener_grad_proj);

            // Momentum gradients
            const auto& r_elem_mom_grad = it_elem->GetValue(MOMENTUM_GRADIENT);
            double mom_grad_norm = 0.0;
            double mom_grad_proj_norm = 0.0;
            for (unsigned int d1 = 0; d1 < dim; ++d1) {
                for (unsigned int d2 = 0; d2 < dim; ++d2) {
                    mom_grad_norm += std::pow(r_elem_mom_grad(d1,d2), 2);
                    mom_grad_proj_norm += std::pow(midpoint_mom_grad_proj(d1,d2), 2);
                }
            }
            mom_grad_norm = std::sqrt(mom_grad_norm);
            mom_grad_proj_norm = std::sqrt(mom_grad_proj_norm);

            // Pressure gradients
            const auto& r_elem_pres_grad = it_elem->GetValue(PRESSURE_GRADIENT);
            const double pres_grad_norm = norm_2(r_elem_pres_grad);
            const double pres_grad_proj_norm = norm_2(midpoint_pres_grad_proj);

            // Density gradients
            const auto& r_elem_rho_grad = it_elem->GetValue(DENSITY_GRADIENT);
            const double rho_grad_norm = norm_2(r_elem_rho_grad);
            const double rho_grad_proj_norm = norm_2(midpoint_rho_grad_proj);

            // Calculate the shock capturing magnitudes
            const double c_a = 0.8;
            const double v_norm = norm_2(midpoint_v);
            const double avg_h = avg_h_function(r_geom);
            const double aux = 0.5 * c_a * v_norm * avg_h;

            const double mom_epsilon = 1.0;
            const double rho_epsilon = 1.0e-4;
            const double pres_epsilon = 1.0e-4;
            const double tot_ener_epsilon = 1.0e-4;

            const double mu = p_prop->GetValue(DYNAMIC_VISCOSITY);
            const double c_v = p_prop->GetValue(SPECIFIC_HEAT);
            const double lambda = p_prop->GetValue(CONDUCTIVITY);

            // Momentum sensor
            const double mom_sensor = std::abs(mom_grad_norm - mom_grad_proj_norm) / (mom_grad_norm + mom_grad_proj_norm + mom_epsilon * (1.0 + norm_2(midpoint_m) / avg_h));
            it_elem->SetValue(MOMENTUM_SHOCK_SENSOR, mom_sensor);

            // Total energy sensor
            const double tot_ener_sensor = std::abs(tot_ener_grad_norm - tot_ener_grad_proj_norm) / (tot_ener_grad_norm + tot_ener_grad_proj_norm + tot_ener_epsilon * (1.0 + midpoint_tot_ener / avg_h));
            it_elem->SetValue(TOTAL_ENERGY_SHOCK_SENSOR, tot_ener_sensor);

            // Pressure sensor
            const double pres_sensor = std::abs(pres_grad_norm - pres_grad_proj_norm) / (pres_grad_norm + pres_grad_proj_norm + pres_epsilon * (midpoint_pres / avg_h));
            it_elem->SetValue(SHOCK_SENSOR, pres_sensor);

            // Density sensor
            const double rho_sensor = std::abs(rho_grad_norm - rho_grad_proj_norm) / (rho_grad_norm + rho_grad_proj_norm + rho_epsilon * (midpoint_rho / avg_h));
            it_elem->SetValue(DENSITY_SHOCK_SENSOR, rho_sensor);

            // Artificial diffusion calculation
            const double max_artificial_viscosity_ratio = 10.0;
            const double max_artificial_conductivity_ratio = 10.0;
            it_elem->GetValue(SHOCK_CAPTURING_VISCOSITY) = std::min(aux * mom_sensor, max_artificial_viscosity_ratio * mom_sensor * mu / midpoint_rho);
            it_elem->GetValue(SHOCK_CAPTURING_CONDUCTIVITY) = std::min(aux * rho_sensor, max_artificial_conductivity_ratio * rho_sensor * lambda / midpoint_rho / c_v);
        }
    }

    void ApplySlipCondition()
    {
        // Calculate the model part data
        auto &r_model_part = BaseType::GetModelPart();
        const int n_nodes = r_model_part.NumberOfNodes();

        // Calculate and substract the normal contribution
#pragma omp parallel for
        for (unsigned int i_node = 0; i_node < n_nodes; ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;
            if (it_node->Is(SLIP)) {
                auto unit_normal = it_node->FastGetSolutionStepValue(NORMAL);
                unit_normal /= norm_2(unit_normal);
                auto& r_mom = it_node->FastGetSolutionStepValue(MOMENTUM);
                const double r_mom_n = inner_prod(r_mom, unit_normal);
                r_mom -= r_mom_n * unit_normal;
            }
        }
    }

    void CalculateValuesSmoothing()
    {
        // Calculate the model part data
        auto &r_model_part = BaseType::GetModelPart();
        const int n_nodes = r_model_part.NumberOfNodes();
        const int n_elems = r_model_part.NumberOfElements();
        const int dim = r_model_part.GetProcessInfo()[DOMAIN_SIZE];
        const unsigned int block_size = dim + 2;

        // Get the required data from the explicit builder and solver
        const auto p_explicit_bs = BaseType::pGetExplicitBuilder();
        auto &r_dof_set = p_explicit_bs->GetDofSet();
        const unsigned int dof_size = p_explicit_bs->GetEquationSystemSize();
        const auto &r_lumped_mass_vector = p_explicit_bs->GetLumpedMassMatrixVector();

        // Set the functor to calculate the element size
        // Note that this assumes a unique geometry in the computational mesh
        std::function<double(Geometry<Node<3>>& rGeometry)> avg_h_function;
        const GeometryData::KratosGeometryType geometry_type = (r_model_part.ElementsBegin()->GetGeometry()).GetGeometryType();
        switch (geometry_type) {
            case GeometryData::KratosGeometryType::Kratos_Triangle2D3:
                avg_h_function = [&](Geometry<Node<3>>& rGeometry){return ElementSizeCalculator<2,3>::AverageElementSize(rGeometry);};
                break;
            case GeometryData::KratosGeometryType::Kratos_Tetrahedra3D4:
                avg_h_function = [&](Geometry<Node<3>>& rGeometry){return ElementSizeCalculator<3,4>::AverageElementSize(rGeometry);};
                break;
            default:
                KRATOS_ERROR << "Asking for a non-implemented geometry.";
        }

        // Initialize smoothed values
#pragma omp parallel for
        for (unsigned int i_node = 0; i_node < n_nodes; ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;
            it_node->GetValue(SMOOTHED_DENSITY) = 0.0;
            it_node->GetValue(SMOOTHED_TOTAL_ENERGY) = 0.0;
            it_node->GetValue(SMOOTHED_MOMENTUM) = ZeroVector(3);
        }

        const double dt = BaseType::GetDeltaTime();
        const double c_e = 1.0; // User specified constant between 0.0 and 2.0

        // Assemble the corrections contributions
        Vector p_grad;
        Geometry<Node<3>>::ShapeFunctionsGradientsType dNdX_container;
#pragma omp parallel for private(p_grad, dNdX_container)
        for (int i_elem = 0; i_elem < n_elems; ++i_elem) {
            auto it_elem = r_model_part.ElementsBegin() + i_elem;
            auto& r_geom = it_elem->GetGeometry();
            const unsigned int n_nodes = r_geom.PointsNumber();
            const double geom_domain_size = r_geom.DomainSize();

            // Calculate the gradients in the element midpoint
            // Note that it is assumed that simplicial elements are used
            dNdX_container = r_geom.ShapeFunctionsIntegrationPointsGradients(dNdX_container, GeometryData::GI_GAUSS_1);
            const auto &r_dNdX = dNdX_container[0];

            // Calculate the required average values
            double p_avg = 0.0;
            double c_avg = 0.0;
            double v_norm_avg = 0.0;
            p_grad = ZeroVector(dim);
            const double gamma = (it_elem->GetProperties()).GetValue(HEAT_CAPACITY_RATIO);
            for (unsigned int i_node = 0; i_node < n_nodes; ++i_node) {
                auto &r_node = r_geom[i_node];
                const auto node_dNdX = row(r_dNdX, i_node);
                const auto &r_mom = r_node.FastGetSolutionStepValue(MOMENTUM);
                const double &r_rho = r_node.FastGetSolutionStepValue(DENSITY);
                const double &r_tot_ener = r_node.FastGetSolutionStepValue(TOTAL_ENERGY);
                const double v_norm = norm_2(r_mom / r_rho);
                const double p = (gamma - 1.0) * (r_tot_ener - 0.5 * std::pow(v_norm, 2) * r_rho);
                const double c = std::sqrt(gamma * p / r_rho);
                p_avg += p;
                c_avg += c;
                v_norm_avg += v_norm;
                for (int d1 = 0; d1 < dim; ++d1) {
                    p_grad(d1) += node_dNdX(d1) * p;
                }
            }
            p_avg /= n_nodes;
            c_avg /= n_nodes;
            v_norm_avg /= n_nodes;
            const double p_grad_norm = norm_2(p_grad);

            // Calculate the multiplying constant
            const double avg_h = avg_h_function(r_geom);
            const double constant = dt * c_e * std::pow(avg_h, 2) * (v_norm_avg + c_avg) * p_grad_norm / p_avg;
            // const double constant = dt * c_e * std::pow(avg_h, 2) * p_grad_norm / p_avg;

            // Elemental diffusive assembly
            for (unsigned int d = 0; d < dim; ++d) {
                for (unsigned int i_node = 0; i_node < n_nodes; ++i_node) {
                    // Get smoothed values
                    auto &r_node_i = r_geom[i_node];
                    auto &r_smooth_mom = r_node_i.GetValue(SMOOTHED_MOMENTUM);
                    double &r_smooth_rho = r_node_i.GetValue(SMOOTHED_DENSITY);
                    double &r_smooth_tot_ener = r_node_i.GetValue(SMOOTHED_TOTAL_ENERGY);
                    // Calculate characteristic velocity
                    const auto& r_rho = r_node_i.FastGetSolutionStepValue(DENSITY);
                    const auto& r_tot_ener = r_node_i.FastGetSolutionStepValue(TOTAL_ENERGY);
                    // const double v_norm = norm_2(r_node_i.FastGetSolutionStepValue(MOMENTUM) / r_rho);
                    // const double p = (gamma - 1.0) * (r_tot_ener - 0.5 * std::pow(v_norm, 2) * r_rho);
                    // const double c = std::sqrt(gamma * p / r_rho);
                    // const double char_vel = v_norm + c;
                    // Calculate the diffusive elemental contribution
                    const double aux_i = r_dNdX(i_node, d);
                    for (unsigned int j_node = 0; j_node < n_nodes; ++j_node) {
                        const auto& r_node_j = r_geom[j_node];
                        const auto &r_mom = r_node_j.FastGetSolutionStepValue(MOMENTUM, 1);
                        const double &r_rho = r_node_j.FastGetSolutionStepValue(DENSITY, 1);
                        const double &r_tot_ener = r_node_j.FastGetSolutionStepValue(TOTAL_ENERGY, 1);
                        r_smooth_rho += aux_i * r_dNdX(j_node, d) * r_rho;
                        r_smooth_mom += aux_i * r_dNdX(j_node, d) * r_mom;
                        r_smooth_tot_ener += aux_i * r_dNdX(j_node, d) * r_tot_ener;
                    }
                    r_smooth_rho *= constant * geom_domain_size;
                    r_smooth_mom *= constant * geom_domain_size;
                    r_smooth_tot_ener *= constant * geom_domain_size;
                    // r_smooth_rho *= constant * char_vel * geom_domain_size;
                    // r_smooth_mom *= constant * char_vel * geom_domain_size;
                    // r_smooth_tot_ener *= constant * char_vel * geom_domain_size;
                }
            }
        }

            // Divide the smoothing contribution by the mass and add to the current solution
#pragma omp parallel for
        for (int i_node = 0; i_node < n_nodes; ++i_node) {
            auto it_node = r_model_part.NodesBegin() + i_node;
            const double mass = r_lumped_mass_vector(i_node * block_size);
            auto& r_smooth_mom = it_node->GetValue(SMOOTHED_MOMENTUM);
            double& r_smooth_rho = it_node->GetValue(SMOOTHED_DENSITY);
            double& r_smooth_tot_ener = it_node->GetValue(SMOOTHED_TOTAL_ENERGY);
            r_smooth_rho /= mass;
            r_smooth_mom /= mass;
            r_smooth_tot_ener /= mass;
            if (!it_node->IsFixed(DENSITY)) {
                it_node->FastGetSolutionStepValue(DENSITY) += r_smooth_rho;
            }
            if (!it_node->IsFixed(MOMENTUM_X)) {
                it_node->FastGetSolutionStepValue(MOMENTUM_X) += r_smooth_mom(0);
            }
            if (!it_node->IsFixed(MOMENTUM_Y)) {
                it_node->FastGetSolutionStepValue(MOMENTUM_Y) += r_smooth_mom(1);
            }
            if (!it_node->IsFixed(MOMENTUM_Z)) {
                it_node->FastGetSolutionStepValue(MOMENTUM_Z) += r_smooth_mom(2);
            }
            if (!it_node->IsFixed(TOTAL_ENERGY)) {
                it_node->FastGetSolutionStepValue(TOTAL_ENERGY) += r_smooth_tot_ener;
            }
        }
    }

    // /**
    //  * @brief Initializes the shock capturing method
    //  * This function performs all the operations required to initialize the shock capturing method
    //  * Note that if the mesh deforms (or changes the topology) it is required to repeat them again
    //  */
    // void InitializeShockCapturing()
    // {
    //     auto& r_model_part = BaseType::GetModelPart();

    //     // Calculate the nodal element size
    //     // This will be used in the calculation of the artificial shock capturing magnitudes
    //     auto nodal_h_process = FindNodalHProcess<FindNodalHSettings::SaveAsNonHistoricalVariable>(r_model_part);
    //     nodal_h_process.Execute();

    //     // Initialize the shock detection process
    //     mpShockDetectionProcess = Kratos::make_unique<ShockDetectionProcess>(r_model_part, PRESSURE, DENSITY_GRADIENT);
    //     // mpShockDetectionProcess = Kratos::make_unique<ShockDetectionProcess>(r_model_part, DENSITY, DENSITY_GRADIENT);
    //     mpShockDetectionProcess->ExecuteInitialize();
    // }

//     /**
//      * @brief Calculates the artificial shock capturing values
//      * This method computes the values of the artificial shock capturing viscosity
//      * and conductivity (note that in here we assume dynamic artificial viscosity)
//      */
//     void CalculateShockCapturingMagnitudes()
//     {
//         // Get model part data
//         auto& r_model_part = BaseType::GetModelPart();
//         auto& r_comm = r_model_part.GetCommunicator();

//         // Calculate the corresponding artificial magnitudes
//         array_1d<double,3> aux_vel;
// #pragma omp parallel for private(aux_vel)
//         for (int i_node = 0; i_node < r_comm.LocalMesh().NumberOfNodes(); ++i_node) {
//             auto it_node = r_comm.LocalMesh().NodesBegin() + i_node;
//             const double& r_nodal_h = it_node->GetValue(NODAL_H);
//             const double& r_shock_sensor = it_node->GetValue(SHOCK_SENSOR);
//             const double& r_rho = it_node->FastGetSolutionStepValue(DENSITY);
//             const auto& r_mom = it_node->FastGetSolutionStepValue(MOMENTUM);
//             aux_vel[0] = r_mom[0] / r_rho;
//             aux_vel[1] = r_mom[1] / r_rho;
//             aux_vel[2] = r_mom[2] / r_rho;
//             double& r_mu_sc = it_node->GetValue(SHOCK_CAPTURING_VISCOSITY);
//             r_mu_sc = r_shock_sensor * r_nodal_h * norm_2(aux_vel); // Kinematic shock capturing viscosity (this is the one implemented in the element right now)
//             // r_mu_sc = r_shock_sensor * r_nodal_h * norm_2(aux_vel) * r_rho; // Dynamic shock capturing viscosity
//         }

// LAPIDUS VISCOSITY
//         // If required recompute the NODAL_AREA
//         // This is required for the nodal gradients calculation
//         CalculateNodalAreaProcess<CalculateNodalAreaSettings::SaveAsNonHistoricalVariable>(
//         r_model_part,
//         r_model_part.GetProcessInfo().GetValue(DOMAIN_SIZE)).Execute();

//         array_1d<double,3> aux_vel;
// #pragma omp parallel for private(aux_vel)
//         for (int i_node = 0; i_node < r_comm.LocalMesh().NumberOfNodes(); ++i_node) {
//             auto it_node = r_comm.LocalMesh().NodesBegin() + i_node;
//             const double& r_rho = it_node->FastGetSolutionStepValue(DENSITY);
//             const auto& r_mom = it_node->FastGetSolutionStepValue(MOMENTUM);
//             aux_vel[0] = r_mom[0] / r_rho;
//             aux_vel[1] = r_mom[1] / r_rho;
//             aux_vel[2] = r_mom[2] / r_rho;
//             it_node->GetValue(CHARACTERISTIC_VELOCITY) = norm_2(aux_vel);
//         }

//         // Calculate the shock variable nodal gradients
//         ComputeNodalGradientProcess<ComputeNodalGradientProcessSettings::SaveAsNonHistoricalVariable>(
//             r_model_part,
//             CHARACTERISTIC_VELOCITY,
//             DENSITY_GRADIENT,
//             NODAL_AREA,
//             true).Execute();

// #pragma omp parallel for private(aux_vel)
//         for (int i_node = 0; i_node < r_comm.LocalMesh().NumberOfNodes(); ++i_node) {
//             auto it_node = r_comm.LocalMesh().NodesBegin() + i_node;
//             auto& r_l = it_node->GetValue(DENSITY_GRADIENT);
//             const double norm_l = norm_2(r_l);
//             if (norm_l > 1.0e-12) {
//                 r_l = r_l / norm_2(r_l);
//             } else {
//                 r_l = ZeroVector(3);
//             }
//             const double& r_rho = it_node->FastGetSolutionStepValue(DENSITY);
//             const auto& r_mom = it_node->FastGetSolutionStepValue(MOMENTUM);
//             aux_vel[0] = r_mom[0] / r_rho;
//             aux_vel[1] = r_mom[1] / r_rho;
//             aux_vel[2] = r_mom[2] / r_rho;
//             it_node->GetValue(CHARACTERISTIC_VELOCITY) = inner_prod(aux_vel, r_l);
//         }

//         // Calculate the shock variable nodal gradients
//         ComputeNodalGradientProcess<ComputeNodalGradientProcessSettings::SaveAsNonHistoricalVariable>(
//             r_model_part,
//             CHARACTERISTIC_VELOCITY,
//             VELOCITY,
//             NODAL_AREA,
//             true).Execute();

//         const double c_lap = 1.0;
// #pragma omp parallel for firstprivate(c_lap)
//         for (int i_node = 0; i_node < r_comm.LocalMesh().NumberOfNodes(); ++i_node) {
//             auto it_node = r_comm.LocalMesh().NodesBegin() + i_node;
//             const double& r_nodal_h = it_node->GetValue(NODAL_H);
//             const auto& r_v_l = it_node->GetValue(VELOCITY);
//             const auto& r_l = it_node->GetValue(DENSITY_GRADIENT);
//             double& r_nu_sc = it_node->GetValue(SHOCK_CAPTURING_VISCOSITY);
//             r_nu_sc =  c_lap * std::pow(r_nodal_h,2) * std::abs(inner_prod(r_l, r_v_l));
//         }
//
//    }

    ///@}
    ///@name Private  Access
    ///@{


    ///@}
    ///@name Private Inquiry
    ///@{


    ///@}
    ///@name Un accessible methods
    ///@{


    ///@}
}; /* Class CompressibleNavierStokesExplicitSolvingStrategyRungeKutta4 */

///@}

///@name Type Definitions
///@{

///@}

} /* namespace Kratos.*/

#endif /* KRATOS_COMPRESSIBLE_NAVIER_STOKES_EXPLICIT_SOLVING_STRATEGY_RUNGE_KUTTA_4  defined */
