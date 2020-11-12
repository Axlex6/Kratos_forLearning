//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Peter Wilson
//

// System includes

// External includes

// Project includes
#include "feti_dynamic_coupling_utilities.h"
#include "factories/linear_solver_factory.h"


namespace Kratos
{
    FetiDynamicCouplingUtilities::FetiDynamicCouplingUtilities(ModelPart& rInterfaceOrigin,
        ModelPart& rInterFaceDestination, Parameters JsonParameters)
        :mrOriginInterfaceModelPart(rInterfaceOrigin), mrDestinationInterfaceModelPart(rInterFaceDestination),
        mParameters(JsonParameters)
    {
        // Check Json settings are present
        KRATOS_ERROR_IF_NOT(mParameters.Has("origin_newmark_beta")) << "'origin_newmark_beta' was not specified in the CoSim parameters file\n";
        KRATOS_ERROR_IF_NOT(mParameters.Has("origin_newmark_gamma")) << "'origin_newmark_gamma' was not specified in the CoSim parameters file\n";
        KRATOS_ERROR_IF_NOT(mParameters.Has("destination_newmark_beta")) << "'destination_newmark_beta' was not specified in the CoSim parameters file\n";
        KRATOS_ERROR_IF_NOT(mParameters.Has("destination_newmark_gamma")) << "'destination_newmark_gamma' was not specified in the CoSim parameters file\n";
        KRATOS_ERROR_IF_NOT(mParameters.Has("timestep_ratio")) << "'timestep_ratio' was not specified in the CoSim parameters file\n";
        KRATOS_ERROR_IF_NOT(mParameters.Has("equilibrium_variable")) << "'equilibrium_variable' was not specified in the CoSim parameters file\n";
        KRATOS_ERROR_IF_NOT(mParameters.Has("is_disable_coupling")) << "'is_disable_coupling' was not specified in the CoSim parameters file\n";

        // Check Json settings are correct
        const double origin_beta = mParameters["origin_newmark_beta"].GetDouble();
        const double origin_gamma = mParameters["origin_newmark_gamma"].GetDouble();
        const double destination_beta = mParameters["destination_newmark_beta"].GetDouble();
        const double destination_gamma = mParameters["destination_newmark_gamma"].GetDouble();
        const double timestep_ratio = mParameters["timestep_ratio"].GetDouble();
        mEquilibriumVariableString = mParameters["equilibrium_variable"].GetString();

        KRATOS_ERROR_IF(origin_beta < 0.0 || origin_beta > 1.0) << "'origin_newmark_beta' has invalid value. It must be between 0 and 1.\n";
        KRATOS_ERROR_IF(origin_gamma < 0.0 || origin_gamma > 1.0) << "'origin_newmark_gamma' has invalid value. It must be between 0 and 1.\n";
        KRATOS_ERROR_IF(destination_beta < 0.0 || destination_beta > 1.0) << "'destination_beta' has invalid value. It must be between 0 and 1.\n";
        KRATOS_ERROR_IF(destination_gamma < 0.0 || destination_gamma > 1.0) << "'destination_gamma' has invalid value. It must be between 0 and 1.\n";
        KRATOS_ERROR_IF(timestep_ratio < 0.0 || std::abs(timestep_ratio - double(int(timestep_ratio))) > numerical_limit) << "'timestep_ratio' has invalid value. It must be a positive integer.\n";
        KRATOS_ERROR_IF(mEquilibriumVariableString != "VELOCITY" && mEquilibriumVariableString != "DISPLACEMENT" && mEquilibriumVariableString != "ACCELERATION") << "'equilibrium_variable' has invalid value. It must be either DISPLACEMENT, VELOCITY or ACCELERATION.\n";

        // Limit only to implicit average acceleration or explicit central difference
        KRATOS_ERROR_IF(origin_beta != 0.0 && origin_beta != 0.25) << "origin_beta must be 0.0 or 0.25";
        KRATOS_ERROR_IF(destination_beta != 0.0 && destination_beta != 0.25) << "destination_beta must be 0.0 or 0.25";
        KRATOS_ERROR_IF(origin_gamma != 0.5) << "origin_gamma must be 0.5";
        KRATOS_ERROR_IF(destination_gamma != 0.5) << "destination_gamma must be 0.5";

        mIsImplicitOrigin = (origin_beta > numerical_limit) ? true : false;
        mIsImplicitDestination = (destination_beta > numerical_limit) ? true : false;
        mTimestepRatio = timestep_ratio;

        mIsLinear = mParameters["is_linear"].GetBool();

        mSubTimestepIndex = 1;
    }

	void FetiDynamicCouplingUtilities::EquilibrateDomains()
	{
        KRATOS_TRY

        // 0 - Setup and checks
        KRATOS_ERROR_IF(mSubTimestepIndex > mTimestepRatio)
        << "FetiDynamicCouplingUtilities::EquilibrateDomains | SubTimestep index incorrectly exceeds timestep ratio.\n";

        KRATOS_ERROR_IF(mpOriginDomain == nullptr || mpDestinationDomain == nullptr)
        << "FetiDynamicCouplingUtilities::EquilibrateDomains | Origin and destination domains have not been set.\n"
        << "Please call 'SetOriginAndDestinationDomainsWithInterfaceModelParts' from python before calling 'EquilibrateDomains'.\n";

        KRATOS_ERROR_IF(mpSolver == nullptr)
            << "FetiDynamicCouplingUtilities::EquilibrateDomains | The linear solver has not been set.\n"
            << "Please call 'SetLinearSolver' from python before calling 'EquilibrateDomains'.\n";

        const SizeType dim_origin = mpOriginDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension();

        KRATOS_ERROR_IF_NOT(mpDestinationDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension() == dim_origin)
            << "FetiDynamicCouplingUtilities::EquilibrateDomains | Origin and destination working space dimensions do not match\n";

        const SizeType destination_interface_dofs = dim_origin * mrDestinationInterfaceModelPart.NumberOfNodes();

        // 1 - calculate unbalanced interface free kinematics
        Vector unbalanced_interface_free_kinematics(destination_interface_dofs);
        CalculateUnbalancedInterfaceFreeKinematics(unbalanced_interface_free_kinematics);

        if (!mIsLinear || !mIsLinearSetupComplete)
        {
            // 2 - Construct projection matrices
            if (mSubTimestepIndex == 1) ComposeProjector(mProjectorOrigin, true);
            const SizeType destination_dofs = (mIsImplicitDestination) ? mpKDestination->size1() : 1;
            if (mProjectorDestination.size1() != destination_interface_dofs || mProjectorDestination.size2() != destination_dofs)
                mProjectorDestination.resize(destination_interface_dofs, destination_dofs);
            ComposeProjector(mProjectorDestination, false);

            // 3 - Determine domain response to unit loads
            if (mSubTimestepIndex == 1) DetermineDomainUnitAccelerationResponse(mpKOrigin, mProjectorOrigin, mUnitResponseOrigin, true);
            if (mUnitResponseDestination.size1() != mProjectorDestination.size2() || mUnitResponseDestination.size2() != mProjectorDestination.size1())
                mUnitResponseDestination.resize(mProjectorDestination.size2(), mProjectorDestination.size1());
            DetermineDomainUnitAccelerationResponse(mpKDestination, mProjectorDestination, mUnitResponseDestination, false);

            // 4 - Calculate condensation matrix
            if (mCondensationMatrix.size1() != destination_interface_dofs || mCondensationMatrix.size2() != destination_interface_dofs)
                mCondensationMatrix.resize(destination_interface_dofs, destination_interface_dofs);
            CalculateCondensationMatrix(mCondensationMatrix, mUnitResponseOrigin,
                mUnitResponseDestination, mProjectorOrigin, mProjectorDestination);

            if (mIsLinear) mIsLinearSetupComplete = true;
        }

        // 5 - Calculate lagrange mults
        Vector lagrange_vector(destination_interface_dofs);
        DetermineLagrangianMultipliers(lagrange_vector, mCondensationMatrix, unbalanced_interface_free_kinematics);
        if (mParameters["is_disable_coupling"].GetBool()) lagrange_vector.clear();
        if (mParameters["is_disable_coupling"].GetBool()) std::cout << "[WARNING] Lagrangian multipliers disabled\n";

        // 6 - Apply correction quantities
        if (mSubTimestepIndex == mTimestepRatio) {
            SetOriginInitialKinematics(); // the final free kinematics of A is the initial free kinematics of A for the next timestep
            ApplyCorrectionQuantities(lagrange_vector, mUnitResponseOrigin, true);
        }
        ApplyCorrectionQuantities(lagrange_vector, mUnitResponseDestination, false);

        // 7 - Optional check of equilibrium
        if (mIsCheckEquilibrium && !mParameters["is_disable_coupling"].GetBool() && mSubTimestepIndex == mTimestepRatio)
        {
            unbalanced_interface_free_kinematics.clear();
            CalculateUnbalancedInterfaceFreeKinematics(unbalanced_interface_free_kinematics, true);
            const double equilibrium_norm = norm_2(unbalanced_interface_free_kinematics);
            KRATOS_ERROR_IF(equilibrium_norm > 1e-12)
                << "FetiDynamicCouplingUtilities::EquilibrateDomains | Corrected interface velocities are not in equilibrium!\n"
                << "Equilibrium norm = " << equilibrium_norm << "\nUnbalanced interface vel = \n"
                << unbalanced_interface_free_kinematics << "\n";
        }

        // 8 - Write nodal lagrange multipliers to interface
        WriteLagrangeMultiplierResults(lagrange_vector);

        // 9 - Advance subtimestep counter
        if (mSubTimestepIndex == mTimestepRatio) mSubTimestepIndex = 1;
        else  mSubTimestepIndex += 1;

        KRATOS_CATCH("")
	}


	void FetiDynamicCouplingUtilities::CalculateUnbalancedInterfaceFreeKinematics(Vector& rUnbalancedKinematics,
        const bool IsEquilibriumCheck)
	{
        KRATOS_TRY

        const SizeType dim = mpOriginDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension();
        Variable< array_1d<double, 3> >& equilibrium_variable = GetEquilibriumVariable();

        // Get destination kinematics
        GetInterfaceQuantity(mrDestinationInterfaceModelPart, equilibrium_variable, rUnbalancedKinematics, dim);
        rUnbalancedKinematics *= -1.0;

        // Get final predicted origin kinematics
        if (mSubTimestepIndex == 1 || IsEquilibriumCheck)
            GetInterfaceQuantity(mrOriginInterfaceModelPart, equilibrium_variable, mFinalOriginInterfaceKinematics, dim);

        // Interpolate origin kinematics to the current sub-timestep
        const double time_ratio = double(mSubTimestepIndex) / double(mTimestepRatio);
        Vector interpolated_origin_kinematics = (IsEquilibriumCheck) ? mFinalOriginInterfaceKinematics
            : time_ratio * mFinalOriginInterfaceKinematics + (1.0 - time_ratio) * mInitialOriginInterfaceKinematics;
        CompressedMatrix expanded_mapper(mpMappingMatrix->size1() * dim, mpMappingMatrix->size2() * dim, 0.0);
        GetExpandedMappingMatrix(expanded_mapper, dim);
        Vector mapped_interpolated_origin_kinematics(expanded_mapper.size1(), 0.0);
        axpy_prod(expanded_mapper, interpolated_origin_kinematics, mapped_interpolated_origin_kinematics,false);

        // Determine kinematics difference
        rUnbalancedKinematics += mapped_interpolated_origin_kinematics;

        KRATOS_CATCH("")
	}


    void FetiDynamicCouplingUtilities::ComposeProjector(CompressedMatrix& rProjector, const bool IsOrigin)
    {
        KRATOS_TRY

        ModelPart& rMP = (IsOrigin) ? mrOriginInterfaceModelPart : mrDestinationInterfaceModelPart;
        const SystemMatrixType* pK = (IsOrigin) ? mpKOrigin : mpKDestination;
        const double projector_entry = (IsOrigin) ? 1.0 : -1.0;
        auto interface_nodes = rMP.NodesArray();
        const SizeType dim = mpOriginDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension();
        const bool is_implicit = (IsOrigin) ? mIsImplicitOrigin : mIsImplicitDestination;

        SizeType domain_dofs = 0;
        if (is_implicit)
        {
            // Implicit - we can use the system matrix size and equation ordering
            domain_dofs = pK->size1();
        }
        else
        {
            // Explicit - we use the nodes with mass in the domain and ordering is just the node index in the model part
            ModelPart& rDomain = (IsOrigin) ? *mpOriginDomain : *mpDestinationDomain;
            for (auto node_it : rDomain.NodesArray())
            {
                const double nodal_mass = node_it->GetValue(NODAL_MASS);
                if (nodal_mass > numerical_limit)
                {
                    node_it->SetValue(EXPLICIT_EQUATION_ID, domain_dofs);
                    domain_dofs += dim;
                }
            }
        }

        if (rProjector.size1() != interface_nodes.size() * dim ||
            rProjector.size2() != domain_dofs)
            rProjector.resize(interface_nodes.size() * dim, domain_dofs, false);
        rProjector.clear();

        IndexType interface_equation_id;
        IndexType domain_equation_id;

        for (size_t i = 0; i < interface_nodes.size(); i++)
        {
            interface_equation_id = interface_nodes[i]->GetValue(INTERFACE_EQUATION_ID);
            domain_equation_id = (is_implicit)
                ? interface_nodes[i]->GetDof(DISPLACEMENT_X).EquationId()
                : interface_nodes[i]->GetValue(EXPLICIT_EQUATION_ID);

            for (size_t dof_dim = 0; dof_dim < dim; dof_dim++)
            {
                rProjector(interface_equation_id*dim + dof_dim, domain_equation_id + dof_dim) = projector_entry;
            }
        }

        // Incorporate force mapping matrix into projector if it is the origin
        // since the lagrangian multipliers are defined on the destination and need
        // to be mapped back later
        if (IsOrigin) ApplyMappingMatrixToProjector(rProjector, dim);

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::CalculateCondensationMatrix(
        CompressedMatrix& rCondensationMatrix,
        const CompressedMatrix& rOriginUnitResponse, const CompressedMatrix& rDestinationUnitResponse,
        const CompressedMatrix& rOriginProjector, const CompressedMatrix& rDestinationProjector)
    {
        KRATOS_TRY

        const double origin_gamma = mParameters["origin_newmark_gamma"].GetDouble();
        const double dest_gamma = mParameters["destination_newmark_gamma"].GetDouble();

        const double origin_dt = mpOriginDomain->GetProcessInfo()[DELTA_TIME];
        const double dest_dt = mpDestinationDomain->GetProcessInfo().GetValue(DELTA_TIME);
        double origin_kinematic_coefficient = 0.0;
        double dest_kinematic_coefficient = 0.0;

        if (mEquilibriumVariableString == "ACCELERATION")
        {
            origin_kinematic_coefficient = 1.0;
            dest_kinematic_coefficient = 1.0;
        }
        else if (mEquilibriumVariableString == "VELOCITY")
        {
            origin_kinematic_coefficient = origin_gamma * origin_dt;
            dest_kinematic_coefficient = dest_gamma * dest_dt;
        }
        else if (mEquilibriumVariableString == "DISPLACEMENT")
        {
            KRATOS_ERROR_IF(mIsImplicitOrigin == false || mIsImplicitDestination == false)
                << "CAN ONLY DO DISPLACEMENT COUPLING FOR IMPLICIT-IMPLICIT";
            origin_kinematic_coefficient = origin_gamma * origin_gamma * origin_dt * origin_dt;
            dest_kinematic_coefficient = dest_gamma * dest_gamma * dest_dt * dest_dt;
        }
        else
        {
            KRATOS_ERROR << "FetiDynamicCouplingUtilities:: The equilibrium variable must be either DISPLACEMENT, VELOCITY or ACCELERATION.";
        }

        CompressedMatrix h_origin(rOriginProjector.size1(), rOriginUnitResponse.size2(),0.0);
        axpy_prod(rOriginProjector, rOriginUnitResponse,h_origin,false); // optimised for sparse matrix prod
        h_origin *= origin_kinematic_coefficient;

        CompressedMatrix h_destination(rDestinationProjector.size1(), rDestinationUnitResponse.size2(), 0.0);
        axpy_prod(rDestinationProjector, rDestinationUnitResponse, h_destination, false); // optimised for sparse matrix prod
        h_destination *= dest_kinematic_coefficient;

        rCondensationMatrix = h_origin + h_destination;
        rCondensationMatrix *= -1.0;

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::DetermineLagrangianMultipliers(Vector& rLagrangeVec,
        CompressedMatrix& rCondensationMatrix, Vector& rUnbalancedKinematics)
    {
        KRATOS_TRY

        if (rLagrangeVec.size() != rUnbalancedKinematics.size())
            rLagrangeVec.resize(rUnbalancedKinematics.size(), false);
        rLagrangeVec.clear();

        mpSolver->Solve(rCondensationMatrix, rLagrangeVec, rUnbalancedKinematics);

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::ApplyCorrectionQuantities(const Vector& rLagrangeVec,
        const CompressedMatrix& rUnitResponse, const bool IsOrigin)
    {
        KRATOS_TRY

        ModelPart* pDomainModelPart = (IsOrigin) ? mpOriginDomain : mpDestinationDomain;
        const double gamma = (IsOrigin) ? mParameters["origin_newmark_gamma"].GetDouble()
            : mParameters["destination_newmark_gamma"].GetDouble();
        const double dt = pDomainModelPart->GetProcessInfo().GetValue(DELTA_TIME);
        const bool is_implicit = (IsOrigin) ? mIsImplicitOrigin : mIsImplicitDestination;

        // Apply acceleration correction
        Vector accel_corrections(rUnitResponse.size1(), 0.0);
        axpy_prod(rUnitResponse, rLagrangeVec, accel_corrections,false);
        AddCorrectionToDomain(pDomainModelPart, ACCELERATION, accel_corrections, is_implicit);

        // Apply velocity correction
        accel_corrections *= (gamma * dt);
        AddCorrectionToDomain(pDomainModelPart, VELOCITY, accel_corrections, is_implicit);

        if (is_implicit)
        {
            // Newmark average acceleration correction
            // gamma = 0.5
            // beta = 0.25 = gamma*gamma
            // deltaAccel = accel_correction
            // deltaVelocity = 0.5 * dt * accel_correction
            // deltaDisplacement = dt * dt * gamma * gamma * accel_correction

            accel_corrections *= (gamma * dt);
            AddCorrectionToDomain(pDomainModelPart, DISPLACEMENT, accel_corrections, is_implicit);
        }
        else
        {
            // FEM central difference correction:
            // gamma = 0.5
            // beta = 0.0
            // deltaAccel = accel_correction
            // deltaVelocity = 0.5 * dt * accel_correction
            // deltaVelocityMiddle = dt * accel_correction
            // deltaDisplacement = dt * dt * accel_correction

            accel_corrections *= 2.0;
            AddCorrectionToDomain(pDomainModelPart, MIDDLE_VELOCITY, accel_corrections, is_implicit);

            // Apply displacement correction
            accel_corrections *= dt;
            AddCorrectionToDomain(pDomainModelPart, DISPLACEMENT, accel_corrections, is_implicit);
        }

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::AddCorrectionToDomain(ModelPart* pDomain,
        const Variable<array_1d<double, 3>>& rVariable, const Vector& rCorrection,
        const bool IsImplicit)
    {
        KRATOS_TRY

        const SizeType dim = mpOriginDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension();

        KRATOS_ERROR_IF_NOT(rCorrection.size() == pDomain->NumberOfNodes() * dim)
            << "AddCorrectionToDomain | Correction dof size does not match domain dofs\n"
            << "Correction size = " << rCorrection.size() << "\n"
            << "Domain dof size = " << pDomain->NumberOfNodes() * dim << "\n"
            << "\n\n\nModel part:\n" << *pDomain << "\n";

        if (IsImplicit)
        {
            block_for_each(pDomain->Nodes(), [&](Node<3>& rNode)
                {
                    IndexType equation_id = rNode.GetDof(DISPLACEMENT_X).EquationId();
                    array_1d<double, 3>& r_nodal_quantity = rNode.FastGetSolutionStepValue(rVariable);
                    for (size_t dof_dim = 0; dof_dim < dim; ++dof_dim)
                        r_nodal_quantity[dof_dim] += rCorrection[equation_id + dof_dim];
                }
            );
        }
        else
        {
            block_for_each(pDomain->Nodes(), [&](Node<3>& rNode)
                {
                    if (rNode.Has(EXPLICIT_EQUATION_ID))
                    {
                        const double nodal_mass = rNode.GetValue(NODAL_MASS);
                        if (nodal_mass > numerical_limit)
                        {
                            IndexType equation_id = rNode.GetValue(EXPLICIT_EQUATION_ID);
                            array_1d<double, 3>& r_nodal_quantity = rNode.FastGetSolutionStepValue(rVariable);
                            for (size_t dof_dim = 0; dof_dim < dim; ++dof_dim)
                            {
                                r_nodal_quantity[dof_dim] += rCorrection[equation_id + dof_dim];
                            }
                        }
                    }
                }
            );
        }

        KRATOS_CATCH("")
    }

    void FetiDynamicCouplingUtilities::WriteLagrangeMultiplierResults(const Vector& rLagrange)
    {
        KRATOS_TRY

        const SizeType dim = mpOriginDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension();

        ModelPart& r_interface = mrDestinationInterfaceModelPart;
        auto interface_nodes = r_interface.NodesArray();
        for (size_t i = 0; i < interface_nodes.size(); ++i)
        {
            IndexType interface_id = interface_nodes[i]->GetValue(INTERFACE_EQUATION_ID);

            array_1d<double, 3>& lagrange = interface_nodes[i]->FastGetSolutionStepValue(VECTOR_LAGRANGE_MULTIPLIER);
            lagrange.clear();
            for (size_t dof = 0; dof < dim; dof++)
            {
                lagrange[dof] = -1.0*rLagrange[interface_id * dim + dof];
            }
        }

        KRATOS_CATCH("")
    }

    void FetiDynamicCouplingUtilities::GetInterfaceQuantity(
        ModelPart& rInterface, const Variable<array_1d<double, 3>>& rVariable,
        Vector& rContainer, const SizeType nDOFs)
    {
        KRATOS_TRY

        if (rContainer.size() != rInterface.NumberOfNodes() * nDOFs)
            rContainer.resize(rInterface.NumberOfNodes() * nDOFs);
        rContainer.clear();

        KRATOS_ERROR_IF_NOT(rInterface.NodesArray()[0]->Has(INTERFACE_EQUATION_ID))
            << "FetiDynamicCouplingUtilities::GetInterfaceQuantity | The interface nodes do not have an interface equation ID.\n"
            << "This is created by the mapper.\n";

        // Fill up container
        block_for_each(rInterface.Nodes(), [&](Node<3>& rNode)
            {
                IndexType interface_id = rNode.GetValue(INTERFACE_EQUATION_ID);
                array_1d<double, 3>& r_quantity = rNode.FastGetSolutionStepValue(rVariable);
                for (size_t dof = 0; dof < nDOFs; dof++)  rContainer[nDOFs * interface_id + dof] = r_quantity[dof];
            }
        );

        KRATOS_CATCH("")
    }

    void FetiDynamicCouplingUtilities::GetInterfaceQuantity(
        ModelPart& rInterface, const Variable<double>& rVariable,
        Vector& rContainer, const SizeType nDOFs)
    {
        KRATOS_TRY

        if (rContainer.size() != rInterface.NumberOfNodes())
            rContainer.resize(rInterface.NumberOfNodes());
        else rContainer.clear();

        KRATOS_ERROR_IF_NOT(rInterface.NodesArray()[0]->Has(INTERFACE_EQUATION_ID))
            << "FetiDynamicCouplingUtilities::GetInterfaceQuantity | The interface nodes do not have an interface equation ID.\n"
            << "This is created by the mapper.\n";

        // Fill up container
        block_for_each(rInterface.Nodes(), [&](Node<3>& rNode)
            {
                IndexType interface_id = rNode.GetValue(INTERFACE_EQUATION_ID);
                rContainer[interface_id] = rNode.FastGetSolutionStepValue(rVariable);
            }
        );

        KRATOS_CATCH("")
    }

    void FetiDynamicCouplingUtilities::GetExpandedMappingMatrix(
        CompressedMatrix& rExpandedMappingMat, const SizeType nDOFs)
    {
        KRATOS_TRY

        if (rExpandedMappingMat.size1() != mpMappingMatrix->size1() * nDOFs ||
            rExpandedMappingMat.size2() != mpMappingMatrix->size2() * nDOFs)
            rExpandedMappingMat.resize(mpMappingMatrix->size1() * nDOFs, mpMappingMatrix->size2() * nDOFs,false);

        rExpandedMappingMat.clear();

        for (size_t dof = 0; dof < nDOFs; dof++)
        {
            for (size_t i = 0; i < mpMappingMatrix->size1(); i++)
            {
                const IndexType row = nDOFs * i + dof;
                for (size_t j = 0; j < mpMappingMatrix->size2(); j++)
                {
                    rExpandedMappingMat(row, nDOFs * j + dof) = (*mpMappingMatrix)(i, j);
                }
            }
        }

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::DetermineDomainUnitAccelerationResponse(
        SystemMatrixType* pK, const CompressedMatrix& rProjector, CompressedMatrix& rUnitResponse,
        const bool isOrigin)
    {
        KRATOS_TRY

        const SizeType interface_dofs = rProjector.size1();
        const SizeType system_dofs = rProjector.size2();
        const bool is_implicit = (isOrigin) ? mIsImplicitOrigin : mIsImplicitDestination;

        if (rUnitResponse.size1() != system_dofs ||
            rUnitResponse.size2() != interface_dofs)
            rUnitResponse.resize(system_dofs, interface_dofs, false);

        rUnitResponse.clear();

        if (is_implicit)
        {
            DetermineDomainUnitAccelerationResponseImplicit(rUnitResponse, rProjector, pK, isOrigin);
        }
        else
        {
            ModelPart& r_domain = (isOrigin) ? *mpOriginDomain : *mpDestinationDomain;
            DetermineDomainUnitAccelerationResponseExplicit(rUnitResponse, rProjector, r_domain, isOrigin);
        }

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::ApplyMappingMatrixToProjector(
        CompressedMatrix& rProjector, const SizeType DOFs)
    {
        KRATOS_TRY

        // expand the mapping matrix to map all dofs at once
        if (mpMappingMatrixForce == nullptr)
        {
            // No force map specified, we use the transpose of the displacement mapper
            // This corresponds to conservation mapping (energy conserved, but approximate force mapping)
            // Note - the combined projector is transposed later, so now we submit trans(trans(M)) = M
            CompressedMatrix expanded_mapper(DOFs * mpMappingMatrix->size1(), DOFs * mpMappingMatrix->size2(), 0.0);
            GetExpandedMappingMatrix(expanded_mapper, DOFs);
            rProjector = prod(expanded_mapper, rProjector);
        }
        else
        {
            KRATOS_ERROR << "Using mpMappingMatrixForce is not yet implemented!\n";
            // Force map has been specified, and we use this.
            // This corresponds to consistent mapping (energy not conserved, but proper force mapping)
            // Note - the combined projector is transposed later, so now we submit trans(trans(M)) = M
            CompressedMatrix expanded_mapper(DOFs * mpMappingMatrixForce->size2(), DOFs * mpMappingMatrixForce->size1(), 0.0);
            GetExpandedMappingMatrix(expanded_mapper, DOFs);
            CompressedMatrix temp(expanded_mapper.size1(), rProjector.size2(), 0.0);
            axpy_prod(expanded_mapper, rProjector, temp,false);
            rProjector = temp;
        }

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::DetermineDomainUnitAccelerationResponseExplicit(CompressedMatrix& rUnitResponse,
        const CompressedMatrix& rProjector, ModelPart& rDomain, const bool isOrigin)
    {
        KRATOS_TRY

        const SizeType interface_dofs = rProjector.size1();
        const SizeType dim = rDomain.ElementsBegin()->GetGeometry().WorkingSpaceDimension();
        auto domain_nodes = rDomain.NodesArray();
        Matrix result(rUnitResponse.size1(), rUnitResponse.size2(), 0.0);

        IndexPartition<>(interface_dofs).for_each([&](SizeType i)
            {
                for (size_t j = 0; j < domain_nodes.size(); ++j)
                {
                    const double nodal_mass = domain_nodes[j]->GetValue(NODAL_MASS);
                    if (nodal_mass > numerical_limit)
                    {
                        IndexType domain_id = domain_nodes[j]->GetValue(EXPLICIT_EQUATION_ID);
                        for (size_t dof = 0; dof < dim; ++dof) result(domain_id + dof, i) =  rProjector(i, domain_id + dof) / nodal_mass;
                    }
                }
            }
        );

        rUnitResponse = CompressedMatrix(result);

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::DetermineDomainUnitAccelerationResponseImplicit(CompressedMatrix& rUnitResponse,
        const CompressedMatrix& rProjector, SystemMatrixType* pK, const bool isOrigin)
    {
        KRATOS_TRY

        const SizeType interface_dofs = rProjector.size1();
        const SizeType system_dofs = rProjector.size2();

        // Convert system stiffness matrix to mass matrix
        const double beta = (isOrigin) ? mParameters["origin_newmark_beta"].GetDouble()
            : mParameters["destination_newmark_beta"].GetDouble();
        const double dt = (isOrigin) ? mpOriginDomain->GetProcessInfo()[DELTA_TIME]
            : mpDestinationDomain->GetProcessInfo()[DELTA_TIME];
        CompressedMatrix effective_mass = (*pK) * (dt * dt * beta);

        //auto start = std::chrono::system_clock::now();
        Matrix result(rUnitResponse.size1(), rUnitResponse.size2(),0.0);

        Parameters solver_parameters(mParameters["linear_solver_settings"]);
        if (!solver_parameters.Has("solver_type")) solver_parameters.AddString("solver_type", "skyline_lu_factorization");

        const int omp_nest = omp_get_nested();
        omp_set_nested(0); // disable omp nesting, forces solvers to be serial

        IndexPartition<>(interface_dofs).for_each([&](SizeType i)
            {
                Vector solution(system_dofs);
                Vector projector_transpose_column(system_dofs);
                auto solver = LinearSolverFactory<SparseSpaceType, LocalSpaceType>().Create(solver_parameters);

                for (size_t j = 0; j < system_dofs; ++j) projector_transpose_column[j] = rProjector(i, j);
                solver->Solve(effective_mass, solution, projector_transpose_column);
                for (size_t j = 0; j < system_dofs; ++j) result(j, i) = solution[j]; // dense matrix for result so we can parallel access
            }
        );

        omp_set_nested(omp_nest);
        rUnitResponse = CompressedMatrix(result);

        //auto end = std::chrono::system_clock::now();
        //auto elasped_solve = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        //std::cout << "response solve time = " << elasped_solve.count() << "\n";

        // reference answer for testing - slow matrix inversion
        const bool is_test_ref = false;
        if (is_test_ref)
        {
            //start = std::chrono::system_clock::now();
            double det;
            Matrix inv(pK->size1(), pK->size2());
            MathUtils<double>::InvertMatrix(*pK, inv, det);
            //end = std::chrono::system_clock::now();
            //auto elasped_invert = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            //std::cout << "solve time = " << elasped_solve.count() << "\n";
            //std::cout << "invert time = " << elasped_invert.count() << "\n";
        }

        KRATOS_CATCH("")
    }


    void FetiDynamicCouplingUtilities::PrintInterfaceKinematics(const Variable< array_1d<double, 3> >& rVariable,
        const bool IsOrigin)
    {
        if (mParameters["echo_level"].GetInt() > 2)
        {
            const SizeType dim_origin = mpOriginDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension();
            const SizeType origin_interface_dofs = dim_origin * mrOriginInterfaceModelPart.NumberOfNodes();
            Vector interface_kinematics(origin_interface_dofs);

            ModelPart& r_interface = (IsOrigin) ? mrOriginInterfaceModelPart : mrDestinationInterfaceModelPart;

            block_for_each(r_interface.Nodes(), [&](Node<3>& rNode)
                {
                    IndexType interface_id = rNode.GetValue(INTERFACE_EQUATION_ID);
                    array_1d<double, 3>& vel = rNode.FastGetSolutionStepValue(rVariable);

                    for (size_t dof = 0; dof < dim_origin; dof++)
                    {
                        interface_kinematics[interface_id * dim_origin + dof] = vel[dof];
                    }
                }
            );

            KRATOS_INFO("FetiDynamicCouplingUtilities") << "Interface " << rVariable.Name() << ", is origin = " << IsOrigin
                << "\n" << interface_kinematics << std::endl;;
        }
    }


    void FetiDynamicCouplingUtilities::SetOriginInitialKinematics()
    {
        KRATOS_TRY

        // Initial checks
        KRATOS_ERROR_IF(mpOriginDomain == nullptr || mpDestinationDomain == nullptr)
        << "FetiDynamicCouplingUtilities::EquilibrateDomains | Origin and destination domains have not been set.\n"
        << "Please call 'SetOriginAndDestinationDomainsWithInterfaceModelParts' from python before calling 'EquilibrateDomains'.\n";

        // Store the initial origin interface velocities
        const SizeType dim_origin = mpOriginDomain->ElementsBegin()->GetGeometry().WorkingSpaceDimension();
        const SizeType origin_interface_dofs = dim_origin * mrOriginInterfaceModelPart.NumberOfNodes();

        if (mInitialOriginInterfaceKinematics.size() != origin_interface_dofs)
            mInitialOriginInterfaceKinematics.resize(origin_interface_dofs);
        mInitialOriginInterfaceKinematics.clear();

        GetInterfaceQuantity(mrOriginInterfaceModelPart, GetEquilibriumVariable(), mInitialOriginInterfaceKinematics, dim_origin);

        KRATOS_CATCH("")
    }

} // namespace Kratos.
