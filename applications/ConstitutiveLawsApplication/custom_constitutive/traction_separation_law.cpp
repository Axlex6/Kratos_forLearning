// KRATOS ___                _   _ _         _   _             __                       _
//       / __\___  _ __  ___| |_(_) |_ _   _| |_(_)_   _____  / /  __ ___      _____   /_\  _ __  _ __
//      / /  / _ \| '_ \/ __| __| | __| | | | __| \ \ / / _ \/ /  / _` \ \ /\ / / __| //_\\| '_ \| '_  |
//     / /__| (_) | | | \__ \ |_| | |_| |_| | |_| |\ V /  __/ /__| (_| |\ V  V /\__ \/  _  \ |_) | |_) |
//     \____/\___/|_| |_|___/\__|_|\__|\__,_|\__|_| \_/ \___\____/\__,_| \_/\_/ |___/\_/ \_/ .__/| .__/
//                                                                                         |_|   |_|
//
//  License:         BSD License
//                     license: structural_mechanics_application/license.txt
//
//  Main authors:    Vicente Mataix Ferrandiz
//                   Fernando Rastellini
//                   Alejandro Cornejo Velazquez
//
// System includes
#include <iostream>
#include <set>

// External includes

// Project includes
#include "includes/checks.h"
#include "custom_constitutive/traction_separation_law.h"
#include "constitutive_laws_application_variables.h"
#include "custom_utilities/tangent_operator_calculator_utility.h"
#include "custom_utilities/advanced_constitutive_law_utilities.h"
#include "custom_utilities/constitutive_law_utilities.h"

namespace Kratos
{
/******************************CONSTRUCTOR******************************************/
/***********************************************************************************/

template<unsigned int TDim>
TractionSeparationLaw3D<TDim>::TractionSeparationLaw3D()
    : BaseType()
{
}

/******************************CONSTRUCTOR******************************************/
/***********************************************************************************/

template<unsigned int TDim>
TractionSeparationLaw3D<TDim>::TractionSeparationLaw3D(const std::vector<double>& rCombinationFactors) : BaseType()
{
    // We compute the proportion of the factors (must be over 1)
    double aux_factor = 0.0;
    for (IndexType i_layer = 0; i_layer < rCombinationFactors.size(); ++i_layer) {
        aux_factor += rCombinationFactors[i_layer];
    }

    KRATOS_ERROR_IF(aux_factor < std::numeric_limits<double>::epsilon()) << "Wrong factors in TractionSeparationLaw3D" << std::endl;

    // Resize
    mCombinationFactors.resize(rCombinationFactors.size());

    // We fill the maps
    for (IndexType i_layer = 0; i_layer < rCombinationFactors.size(); ++i_layer) {
        mCombinationFactors[i_layer] = rCombinationFactors[i_layer]/aux_factor;
    }
}

/******************************COPY CONSTRUCTOR*************************************/
/***********************************************************************************/

template<unsigned int TDim>
TractionSeparationLaw3D<TDim>::TractionSeparationLaw3D(const TractionSeparationLaw3D<TDim>& rOther)
    : BaseType(rOther),
      mConstitutiveLaws(rOther.mConstitutiveLaws),
      mCombinationFactors(rOther.mCombinationFactors),
      mdelamination_damage_mode_one(rOther.mdelamination_damage_mode_one),
      mdelamination_damage_mode_two(rOther.mdelamination_damage_mode_two),
      mthreshold_mode_one(rOther.mthreshold_mode_one),
      mthreshold_mode_two(rOther.mthreshold_mode_two)
{
}

/********************************CLONE**********************************************/
/***********************************************************************************/

template<unsigned int TDim>
ConstitutiveLaw::Pointer TractionSeparationLaw3D<TDim>::Clone() const
{
    return Kratos::make_shared<TractionSeparationLaw3D>(*this);
}

/*******************************CONSTRUCTOR*****************************************/
/***********************************************************************************/

template<unsigned int TDim>
ConstitutiveLaw::Pointer TractionSeparationLaw3D<TDim>::Create(Kratos::Parameters NewParameters) const
{
    // We do some checks
    KRATOS_ERROR_IF_NOT(NewParameters.Has("combination_factors")) << "TractionSeparationLaw3D: Please define combination_factors" << std::endl;

    const SizeType number_of_factors = NewParameters["combination_factors"].size();

    // We create the vectors
    std::vector<double> combination_factors(number_of_factors);

    for (IndexType i_layer = 0; i_layer < number_of_factors; ++i_layer) {
        combination_factors[i_layer] = NewParameters["combination_factors"][i_layer].GetDouble();
    }

    KRATOS_ERROR_IF(number_of_factors == 0) << "Please define the combination factors" << std::endl;

    // We create the law
    return Kratos::make_shared<TractionSeparationLaw3D>(combination_factors);
}

//*******************************DESTRUCTOR*******************************************
/***********************************************************************************/

template<unsigned int TDim>
TractionSeparationLaw3D<TDim>::~TractionSeparationLaw3D()
{
};

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
std::size_t TractionSeparationLaw3D<TDim>::WorkingSpaceDimension()
{
    IndexType counter = 0;
    SizeType dimension = 3;
    if (mConstitutiveLaws.size() == 0) return dimension; // In case of not initialized CL
    // We perform the check in each layer
    for (auto& p_law : mConstitutiveLaws) {
        if (counter == 0) {
            dimension = p_law->WorkingSpaceDimension();
        } else {
            KRATOS_ERROR_IF_NOT(dimension == p_law->WorkingSpaceDimension()) << "Combining different size laws" << std::endl;
        }

        ++counter;
    }

    return dimension;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
std::size_t TractionSeparationLaw3D<TDim>::GetStrainSize() const
{
    IndexType counter = 0;
    SizeType strain_size = 6;
    if (mConstitutiveLaws.size() == 0) return strain_size; // In case of not initialized CL
    // We perform the check in each layer
    for (auto& p_law : mConstitutiveLaws) {
        if (counter == 0) {
            strain_size = p_law->GetStrainSize();
        } else {
            KRATOS_ERROR_IF_NOT(strain_size == p_law->GetStrainSize()) << "Combining different size laws" << std::endl;
        }

        ++counter;
    }

    return strain_size;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::Has(const Variable<bool>& rThisVariable)
{
    // At least one layer should have the value
    bool has = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            has = true;
            break;
        }
    }

    return has;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::Has(const Variable<int>& rThisVariable)
{
    // At least one layer should have the value
    bool has = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            has = true;
            break;
        }
    }

    return has;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::Has(const Variable<double>& rThisVariable)
{
    // At least one layer should have the value
    bool has = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            has = true;
            break;
        }
    }

    return has;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::Has(const Variable<Vector>& rThisVariable)
{
    // At least one layer should have the value
    bool has = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            has = true;
            break;
        }
    }

    if (rThisVariable == DELAMINATION_DAMAGE_VECTOR_MODE_ONE) {
        return true;
    }

    if (rThisVariable == DELAMINATION_DAMAGE_VECTOR_MODE_TWO) {
        return true;
    }
    
    return has;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::Has(const Variable<Matrix>& rThisVariable)
{
    // At least one layer should have the value
    bool has = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            has = true;
            break;
        }
    }

    return has;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::Has(const Variable<array_1d<double, 3 > >& rThisVariable)
{
    // At least one layer should have the value
    bool has = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            has = true;
            break;
        }
    }

    return has;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::Has(const Variable<array_1d<double, 6 > >& rThisVariable)
{
    // At least one layer should have the value
    bool has = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            has = true;
            break;
        }
    }

    return has;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool& TractionSeparationLaw3D<TDim>::GetValue(
    const Variable<bool>& rThisVariable,
    bool& rValue
    )
{
    // At least one layer should have the value
    rValue = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->GetValue(rThisVariable, rValue))
            break;
    }

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
int& TractionSeparationLaw3D<TDim>::GetValue(
    const Variable<int>& rThisVariable,
    int& rValue
    )
{
    // At least one layer should have the value
    rValue = 0;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->Has(rThisVariable)) {
            p_law->GetValue(rThisVariable, rValue);
            break;
        }
    }

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
double& TractionSeparationLaw3D<TDim>::GetValue(
    const Variable<double>& rThisVariable,
    double& rValue
    )
{
    // We combine the values of the layers
    rValue = 0.0;
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        double aux_value;
        p_law->GetValue(rThisVariable, aux_value);
        rValue += aux_value * factor;
    }

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
Vector& TractionSeparationLaw3D<TDim>::GetValue(
    const Variable<Vector>& rThisVariable,
    Vector& rValue
    )
{
    // We combine the values of the layers
    rValue.clear();
    // for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
    //     const double factor = mCombinationFactors[i_layer];
    //     ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

    //     Vector aux_value;
    //     p_law->GetValue(rThisVariable, aux_value);
    //     rValue += aux_value * factor;
    // }

    if (rThisVariable == DELAMINATION_DAMAGE_VECTOR_MODE_ONE) {
        
        rValue.resize(mCombinationFactors.size()+1, false);
        
        noalias(rValue) = mdelamination_damage_mode_one;
        return rValue;
    }

    if (rThisVariable == DELAMINATION_DAMAGE_VECTOR_MODE_TWO) {
        
        rValue.resize(mCombinationFactors.size()+1, false);
        
        noalias(rValue) = mdelamination_damage_mode_two;
        return rValue;
    }
    
    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
Matrix& TractionSeparationLaw3D<TDim>::GetValue(
    const Variable<Matrix>& rThisVariable,
    Matrix& rValue
    )
{
    // We combine the values of the layers
    rValue.clear();
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        Matrix aux_value;
        p_law->GetValue(rThisVariable, aux_value);
        rValue += aux_value * factor;
    }

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
array_1d<double, 3 >& TractionSeparationLaw3D<TDim>::GetValue(
    const Variable<array_1d<double, 3 >>& rThisVariable,
    array_1d<double, 3 >& rValue
    )
{
    // We combine the values of the layers
    rValue = ZeroVector(3);
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        array_1d<double, 3 > aux_value;
        p_law->GetValue(rThisVariable, aux_value);
        rValue += aux_value * factor;
    }

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
array_1d<double, 6 >& TractionSeparationLaw3D<TDim>::GetValue(
    const Variable<array_1d<double, 6 >>& rThisVariable,
    array_1d<double, 6 >& rValue
    )
{
    // We combine the values of the layers
    rValue = ZeroVector(6);
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        array_1d<double, 6 > aux_value;
        p_law->GetValue(rThisVariable, aux_value);
        rValue += aux_value * factor;
    }

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::SetValue(
    const Variable<bool>& rThisVariable,
    const bool& rValue,
    const ProcessInfo& rCurrentProcessInfo
    )
{
    // We set the value in all layers

    for (auto& p_law : mConstitutiveLaws) {
        p_law->SetValue(rThisVariable, rValue, rCurrentProcessInfo);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::SetValue(
    const Variable<int>& rThisVariable,
    const int& rValue,
    const ProcessInfo& rCurrentProcessInfo
    )
{
    // We set the value in all layers

    for (auto& p_law : mConstitutiveLaws) {
        p_law->SetValue(rThisVariable, rValue, rCurrentProcessInfo);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::SetValue(
    const Variable<double>& rThisVariable,
    const double& rValue,
    const ProcessInfo& rCurrentProcessInfo
    )
{
    // We set the propotional value in all layers
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        p_law->SetValue(rThisVariable, factor * rValue, rCurrentProcessInfo);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::SetValue(
    const Variable<Vector>& rThisVariable,
    const Vector& rValue,
    const ProcessInfo& rCurrentProcessInfo
    )
{
    // We set the propotional value in all layers
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        p_law->SetValue(rThisVariable, factor * rValue, rCurrentProcessInfo);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::SetValue(
    const Variable<Matrix>& rThisVariable,
    const Matrix& rValue,
    const ProcessInfo& rCurrentProcessInfo
    )
{
    // We set the propotional value in all layers
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        p_law->SetValue(rThisVariable, factor * rValue, rCurrentProcessInfo);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::SetValue(
    const Variable<array_1d<double, 3 >>& rThisVariable,
    const array_1d<double, 3 >& rValue,
    const ProcessInfo& rCurrentProcessInfo
    )
{
    // We set the propotional value in all layers
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        p_law->SetValue(rThisVariable, factor * rValue, rCurrentProcessInfo);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::SetValue(
    const Variable<array_1d<double, 6 >>& rThisVariable,
    const array_1d<double, 6 >& rValue,
    const ProcessInfo& rCurrentProcessInfo
    )
{
    // We set the propotional value in all layers
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];

        p_law->SetValue(rThisVariable, factor * rValue, rCurrentProcessInfo);
    }
}


/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
double& TractionSeparationLaw3D<TDim>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<double>& rThisVariable,
    double& rValue
    )
{
    const Properties& r_material_properties  = rParameterValues.GetMaterialProperties();

    // We combine the value of each layer
    rValue = 0.0;
    const auto it_prop_begin = r_material_properties.GetSubProperties().begin();
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
        Properties& r_prop = *(it_prop_begin + i_layer);

        rParameterValues.SetMaterialProperties(r_prop);
        double aux_value;
        p_law->CalculateValue(rParameterValues,rThisVariable, aux_value);
        rValue += factor * aux_value;
    }

    // Reset properties
    rParameterValues.SetMaterialProperties(r_material_properties);

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
Vector& TractionSeparationLaw3D<TDim>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<Vector>& rThisVariable,
    Vector& rValue
    )
{
    // We do some special operation for strains and stresses
    if (rThisVariable == STRAIN ||
        rThisVariable == GREEN_LAGRANGE_STRAIN_VECTOR ||
        rThisVariable == ALMANSI_STRAIN_VECTOR) {

        // Get Values to compute the constitutive law:
        Flags& r_flags = rParameterValues.GetOptions();

        // Previous flags saved
        const bool flag_const_tensor = r_flags.Is( BaseType::COMPUTE_CONSTITUTIVE_TENSOR );
        const bool flag_stress = r_flags.Is( BaseType::COMPUTE_STRESS );

        r_flags.Set( BaseType::COMPUTE_CONSTITUTIVE_TENSOR, false );
        r_flags.Set( BaseType::COMPUTE_STRESS, false );

        // We compute the strain
        if (rThisVariable == STRAIN) {
            this->CalculateMaterialResponse(rParameterValues, this->GetStressMeasure());
        } else if (rThisVariable == GREEN_LAGRANGE_STRAIN_VECTOR) {
            this->CalculateMaterialResponsePK2(rParameterValues);
        } else if (rThisVariable == ALMANSI_STRAIN_VECTOR) {
            this->CalculateMaterialResponseKirchhoff(rParameterValues);
        }

        noalias(rValue) = rParameterValues.GetStrainVector();

        // Previous flags restored
        r_flags.Set( BaseType::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor );
        r_flags.Set( BaseType::COMPUTE_STRESS, flag_stress );
    } else if (rThisVariable == STRESSES ||
        rThisVariable == CAUCHY_STRESS_VECTOR ||
        rThisVariable == KIRCHHOFF_STRESS_VECTOR ||
        rThisVariable == PK2_STRESS_VECTOR) {

        // Get Values to compute the constitutive law:
        Flags& r_flags = rParameterValues.GetOptions();

        // Previous flags saved
        const bool flag_const_tensor = r_flags.Is( BaseType::COMPUTE_CONSTITUTIVE_TENSOR );
        const bool flag_stress = r_flags.Is( BaseType::COMPUTE_STRESS );

        r_flags.Set( BaseType::COMPUTE_CONSTITUTIVE_TENSOR, false );
        r_flags.Set( BaseType::COMPUTE_STRESS, true );

        // We compute the stress
        if (rThisVariable == STRESSES) {
            this->CalculateMaterialResponse(rParameterValues, this->GetStressMeasure());
        } if (rThisVariable == KIRCHHOFF_STRESS_VECTOR) {
            this->CalculateMaterialResponseKirchhoff(rParameterValues);
        } if (rThisVariable == CAUCHY_STRESS_VECTOR) {
            this->CalculateMaterialResponseCauchy(rParameterValues);
        } if (rThisVariable == PK2_STRESS_VECTOR) {
            this->CalculateMaterialResponsePK2(rParameterValues);
        }

        noalias(rValue) = rParameterValues.GetStressVector();

        // Previous flags restored
        r_flags.Set( BaseType::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor );
        r_flags.Set( BaseType::COMPUTE_STRESS, flag_stress );
    } else {
        const Properties& r_material_properties  = rParameterValues.GetMaterialProperties();

        // We combine the value of each layer
        rValue.clear();
        const auto it_prop_begin = r_material_properties.GetSubProperties().begin();
        for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
            const double factor = mCombinationFactors[i_layer];
            ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
            Properties& r_prop = *(it_prop_begin + i_layer);

            rParameterValues.SetMaterialProperties(r_prop);
            Vector aux_value;
            p_law->CalculateValue(rParameterValues,rThisVariable, aux_value);
            noalias(rValue) += factor * aux_value;
        }

        // Reset properties
        rParameterValues.SetMaterialProperties(r_material_properties);
    }

    return( rValue );
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
Matrix& TractionSeparationLaw3D<TDim>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<Matrix>& rThisVariable,
    Matrix& rValue
    )
{
    // We do some special operations for constitutive matrices
    if (rThisVariable == CONSTITUTIVE_MATRIX ||
        rThisVariable == CONSTITUTIVE_MATRIX_PK2 ||
        rThisVariable == CONSTITUTIVE_MATRIX_KIRCHHOFF) {
        // Get Values to compute the constitutive law:
        Flags& r_flags = rParameterValues.GetOptions();

        // Previous flags saved
        const bool flag_const_tensor = r_flags.Is( BaseType::COMPUTE_CONSTITUTIVE_TENSOR );
        const bool flag_stress = r_flags.Is( BaseType::COMPUTE_STRESS );

        r_flags.Set( BaseType::COMPUTE_CONSTITUTIVE_TENSOR, true );
        r_flags.Set( BaseType::COMPUTE_STRESS, false );

        // We compute the constitutive matrix
        if (rThisVariable == CONSTITUTIVE_MATRIX) {
            this->CalculateMaterialResponse(rParameterValues, this->GetStressMeasure());
        } else if (rThisVariable == CONSTITUTIVE_MATRIX_PK2) {
            this->CalculateMaterialResponsePK2(rParameterValues);
        } else if (rThisVariable == CONSTITUTIVE_MATRIX_KIRCHHOFF) {
            this->CalculateMaterialResponsePK2(rParameterValues);
        }

        noalias(rValue) = rParameterValues.GetConstitutiveMatrix();

        // Previous flags restored
        r_flags.Set( BaseType::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor );
        r_flags.Set( BaseType::COMPUTE_STRESS, flag_stress );
    } else {
        const Properties& r_material_properties  = rParameterValues.GetMaterialProperties();

        // We combine the value of each layer
        rValue.clear();
        const auto it_prop_begin = r_material_properties.GetSubProperties().begin();
        for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
            const double factor = mCombinationFactors[i_layer];
            ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
            Properties& r_prop = *(it_prop_begin + i_layer);

            rParameterValues.SetMaterialProperties(r_prop);
            Matrix aux_value;
            p_law->CalculateValue(rParameterValues,rThisVariable, aux_value);
            noalias(rValue) += factor * aux_value;
        }

        // Reset properties
        rParameterValues.SetMaterialProperties(r_material_properties);
    }

    return( rValue );
}
/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
array_1d<double, 3 >& TractionSeparationLaw3D<TDim>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<array_1d<double, 3 >>& rThisVariable,
    array_1d<double, 3 >& rValue
    )
{
    const Properties& r_material_properties  = rParameterValues.GetMaterialProperties();

    // We combine the value of each layer
    noalias(rValue) = ZeroVector(3);
    const auto it_prop_begin = r_material_properties.GetSubProperties().begin();
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
        Properties& r_prop = *(it_prop_begin + i_layer);

        rParameterValues.SetMaterialProperties(r_prop);
        array_1d<double, 3 > aux_value;
        p_law->CalculateValue(rParameterValues,rThisVariable, aux_value);
        noalias(rValue) += factor * aux_value;
    }

    // Reset properties
    rParameterValues.SetMaterialProperties(r_material_properties);

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
array_1d<double, 6 >& TractionSeparationLaw3D<TDim>::CalculateValue(
    ConstitutiveLaw::Parameters& rParameterValues,
    const Variable<array_1d<double, 6 >>& rThisVariable,
    array_1d<double, 6 >& rValue
    )
{
    const Properties& r_material_properties  = rParameterValues.GetMaterialProperties();

    // We combine the value of each layer
    noalias(rValue) = ZeroVector(6);
    const auto it_prop_begin = r_material_properties.GetSubProperties().begin();
    for (IndexType i_layer = 0; i_layer < mCombinationFactors.size(); ++i_layer) {
        const double factor = mCombinationFactors[i_layer];
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
        Properties& r_prop = *(it_prop_begin + i_layer);

        rParameterValues.SetMaterialProperties(r_prop);
        array_1d<double, 6 > aux_value;
        p_law->CalculateValue(rParameterValues,rThisVariable, aux_value);
        noalias(rValue) += factor * aux_value;
    }

    // Reset properties
    rParameterValues.SetMaterialProperties(r_material_properties);

    return rValue;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::ValidateInput(const Properties& rMaterialProperties)
{
    // We check it layer by layer
    bool valid_input = true;
    for (IndexType i_layer = 0; i_layer < mConstitutiveLaws.size(); ++i_layer) {
        ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
        Properties& r_prop = *(rMaterialProperties.GetSubProperties().begin() + i_layer);
        if (p_law->ValidateInput(r_prop)) {
            valid_input = false;
            break;
        }
    }

    return valid_input;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
ConstitutiveLaw::StrainMeasure TractionSeparationLaw3D<TDim>::GetStrainMeasure()
{
    // We return the first one
    KRATOS_ERROR_IF(mConstitutiveLaws.size() == 0) << "TractionSeparationLaw3D: No constitutive laws defined" << std::endl;
    return mConstitutiveLaws[0]->GetStrainMeasure();
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
ConstitutiveLaw::StressMeasure TractionSeparationLaw3D<TDim>::GetStressMeasure()
{
    // We return the first one
    KRATOS_ERROR_IF(mConstitutiveLaws.size() == 0) << "TractionSeparationLaw3D: No constitutive laws defined" << std::endl;
    return mConstitutiveLaws[0]->GetStressMeasure();
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
bool TractionSeparationLaw3D<TDim>::IsIncremental()
{
    // We check it layer by layer
    bool is_incremental = false;

    for (auto& p_law : mConstitutiveLaws) {
        if (p_law->IsIncremental()) {
            is_incremental = true;
            break;
        }
    }

    return is_incremental;
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::InitializeMaterial(
    const Properties& rMaterialProperties,
    const ConstitutiveLaw::GeometryType& rElementGeometry,
    const Vector& rShapeFunctionsValues
    )
{
    // Resizing first
    mConstitutiveLaws.resize(mCombinationFactors.size());

    // We create the inner constitutive laws
    const auto it_cl_begin = rMaterialProperties.GetSubProperties().begin();
    for (IndexType i_layer = 0; i_layer < mConstitutiveLaws.size(); ++i_layer) {
        Properties& r_prop = *(it_cl_begin + i_layer);

        KRATOS_ERROR_IF_NOT(r_prop.Has(CONSTITUTIVE_LAW)) << "No constitutive law set" << std::endl;
        mConstitutiveLaws[i_layer] = r_prop[CONSTITUTIVE_LAW]->Clone();
        mConstitutiveLaws[i_layer]->InitializeMaterial(r_prop, rElementGeometry, rShapeFunctionsValues);
    }

    KRATOS_DEBUG_ERROR_IF(mConstitutiveLaws.size() == 0) << "TractionSeparationLaw3D: No CL defined" << std::endl;

    mdelamination_damage_mode_one.resize(mConstitutiveLaws.size()+1, false);
    noalias(mdelamination_damage_mode_one) = ZeroVector(mConstitutiveLaws.size()+1);

    mdelamination_damage_mode_two.resize(mConstitutiveLaws.size()+1, false);
    noalias(mdelamination_damage_mode_two) = ZeroVector(mConstitutiveLaws.size()+1);

    mthreshold_mode_one.resize(mConstitutiveLaws.size()-1, false);
    for (int i=0; i < mConstitutiveLaws.size()-1; ++i) {
            mthreshold_mode_one[i] = rMaterialProperties[INTERFACIAL_NORMAL_STRENGTH];
        }

    mthreshold_mode_two.resize(mConstitutiveLaws.size()-1, false);
    for (int i=0; i < mConstitutiveLaws.size()-1; ++i) {
            mthreshold_mode_two[i] = rMaterialProperties[INTERFACIAL_SHEAR_STRENGTH];
        }
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void  TractionSeparationLaw3D<TDim>::CalculateMaterialResponsePK1(ConstitutiveLaw::Parameters& rValues)
{
    CalculateMaterialResponsePK2(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void  TractionSeparationLaw3D<TDim>::CalculateMaterialResponsePK2(ConstitutiveLaw::Parameters& rValues)
{
    KRATOS_TRY;

    // Get Values to compute the constitutive law:
    Flags& r_flags = rValues.GetOptions();

    // Previous flags saved
    const bool flag_strain       = r_flags.Is(BaseType::USE_ELEMENT_PROVIDED_STRAIN);
    const bool flag_const_tensor = r_flags.Is(BaseType::COMPUTE_CONSTITUTIVE_TENSOR);
    const bool flag_stress       = r_flags.Is(BaseType::COMPUTE_STRESS);

    const Properties& r_material_properties = rValues.GetMaterialProperties();

    // The deformation gradient
    if (rValues.IsSetDeterminantF()) {
        const double determinant_f = rValues.GetDeterminantF();
        KRATOS_ERROR_IF(determinant_f < 0.0) << "Deformation gradient determinant (detF) < 0.0 : " << determinant_f << std::endl;
    }

    // All the strains must be the same, therefore we can just simply compute the strain in the first layer
    if (r_flags.IsNot(BaseType::USE_ELEMENT_PROVIDED_STRAIN)) {
        this->CalculateGreenLagrangeStrain(rValues);
        r_flags.Set(BaseType::USE_ELEMENT_PROVIDED_STRAIN, true);
    }

    // The global strain vector, constant
    const Vector strain_vector = rValues.GetStrainVector();

    if (r_flags.Is(BaseType::COMPUTE_STRESS)) {
        // Set new flags
        r_flags.Set(BaseType::COMPUTE_CONSTITUTIVE_TENSOR, false);
        r_flags.Set(BaseType::COMPUTE_STRESS, true);

        // Auxiliar stress vector
        const auto it_prop_begin       = r_material_properties.GetSubProperties().begin();
        Vector auxiliar_stress_vector  = ZeroVector(VoigtSize);
        Vector delamination_damage_affected_stress_vector  = ZeroVector(VoigtSize);

        // The rotation matrix
        BoundedMatrix<double, VoigtSize, VoigtSize> voigt_rotation_matrix;


        std::vector<Vector> layer_stress(mConstitutiveLaws.size());
        for (int i=0; i < mConstitutiveLaws.size(); ++i) {
            layer_stress[i].resize(6, false);
        }

        std::vector<Vector> interfacial_stress(mConstitutiveLaws.size()-1);
        for (int i=0; i < mConstitutiveLaws.size()-1; ++i) {
            interfacial_stress[i].resize(3, false);
        }

        std::vector<bool> negative_interfacial_stress_indicator(mConstitutiveLaws.size()+1);
        for (int i=0; i < mConstitutiveLaws.size()+1; ++i) {
            negative_interfacial_stress_indicator[i] = false;
        }


        for (IndexType i_layer = 0; i_layer < mConstitutiveLaws.size(); ++i_layer) {

            this->CalculateRotationMatrix(r_material_properties, voigt_rotation_matrix, i_layer);

            Properties& r_prop             = *(it_prop_begin + i_layer);
            ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
            const double factor            = mCombinationFactors[i_layer];

            // We rotate to local axes the strain
            noalias(rValues.GetStrainVector()) = prod(voigt_rotation_matrix, strain_vector);

            rValues.SetMaterialProperties(r_prop);
            p_law->CalculateMaterialResponsePK2(rValues);

            // we return the stress and constitutive tensor to the global coordinates
            rValues.GetStressVector()        = prod(trans(voigt_rotation_matrix), rValues.GetStressVector());
            noalias(layer_stress[i_layer]) = rValues.GetStressVector();

            // we reset the properties and Strain
            rValues.SetMaterialProperties(r_material_properties);
            noalias(rValues.GetStrainVector()) = strain_vector;
        }

        const double tolerance = std::numeric_limits<double>::epsilon();
        Vector delamination_damage_mode_one(mConstitutiveLaws.size()+1);
        Vector delamination_damage_mode_two(mConstitutiveLaws.size()+1);
        Vector threshold_mode_one(mConstitutiveLaws.size()-1);
        Vector threshold_mode_two(mConstitutiveLaws.size()-1);
    
        noalias(delamination_damage_mode_one) = mdelamination_damage_mode_one;
        noalias(delamination_damage_mode_two) = mdelamination_damage_mode_two;
        noalias(threshold_mode_one) = mthreshold_mode_one;
        noalias(threshold_mode_two) = mthreshold_mode_two;


        for(int i=0; i < mConstitutiveLaws.size()-1; ++i) {

            interfacial_stress[i][0] = MacaullyBrackets((layer_stress[i][2] + layer_stress[i+1][2]) * 0.5); // interfacial normal stress
            interfacial_stress[i][1] = (layer_stress[i][4] + layer_stress[i+1][4]) * 0.5; // interfacial shear stress
            interfacial_stress[i][2] = (layer_stress[i][5] + layer_stress[i+1][5]) * 0.5; // interfacial shear stress

            const double equivalent_stress_mode_one = interfacial_stress[i][0];
            const double equivalent_stress_mode_two = std::sqrt(std::pow(interfacial_stress[i][1],2.0)+std::pow(interfacial_stress[i][2],2.0));

            if ((layer_stress[i][2] + layer_stress[i+1][2] * 0.5) < tolerance) {
                negative_interfacial_stress_indicator[i+1] = true;
            }

            // Damage calculation

            const double T0n = r_material_properties[INTERFACIAL_NORMAL_STRENGTH]; // Interfacial Normal Strength
            const double T0s = r_material_properties[INTERFACIAL_SHEAR_STRENGTH]; // Interfacial Shear Strength
            const double T0t = r_material_properties[INTERFACIAL_SHEAR_STRENGTH]; // Interfacial Shear Strength
            const double GIc = r_material_properties[MODE_ONE_FRACTURE_ENERGY]; // Mode I Energy Release Rate
            const double GIIc = r_material_properties[MODE_TWO_FRACTURE_ENERGY]; // Mode II Energy Release Rate
            const double Ei = r_material_properties[TENSILE_INTERFACE_MODULUS]; // Tensile modulus of the interface
            const double Gi = r_material_properties[SHEAR_INTERFACE_MODULUS]; // Shear modulus of the interface
            const double characteristic_length = 0.6343 * (AdvancedConstitutiveLawUtilities<VoigtSize>::CalculateCharacteristicLengthOnReferenceConfiguration(rValues.GetElementGeometry()));
        
            const double F_mode_one = equivalent_stress_mode_one - threshold_mode_one[i];
            if (F_mode_one > tolerance) {

                // const double AParameter_mode_one = -std::pow(T0n, 2) / (2.0 * Ei * GIc / characteristic_length); // Linear
                const double AParameter_mode_one = 1.00 / (GIc * Ei / (characteristic_length * std::pow(T0n, 2)) - 0.5); // Exponential
                
                KRATOS_ERROR_IF(AParameter_mode_one < 0.0) << "AParameter_mode_one is negative." << std::endl;

                // delamination_damage_mode_one[i+1] = (1.0 - T0n / equivalent_stress_mode_one) / (1.0 + AParameter_mode_one); // Linear
                delamination_damage_mode_one[i+1] = 1.0 - (T0n / equivalent_stress_mode_one) * std::exp(AParameter_mode_one *
                    (1.0 - equivalent_stress_mode_one / T0n)); // Exponential

                delamination_damage_mode_one[i+1] = (delamination_damage_mode_one[i+1] >= 0.99999) ? 0.99999 : delamination_damage_mode_one[i+1];
                delamination_damage_mode_one[i+1] = (delamination_damage_mode_one[i+1] < 0.0) ? 0.0 : delamination_damage_mode_one[i+1];
            }

            const double F_mode_two = equivalent_stress_mode_two - threshold_mode_two[i];
            if (F_mode_two > tolerance) {

                // const double AParameter_mode_two = -std::pow(T0s, 2) / (2.0 * Gi * GIIc / characteristic_length); // Linear
                const double AParameter_mode_two = 1.00 / (GIIc * Gi / (characteristic_length * std::pow(T0s, 2)) - 0.5); // Exponential
                
                KRATOS_ERROR_IF(AParameter_mode_two < 0.0) << "AParameter_mode_two is negative." << std::endl;

                // delamination_damage_mode_two[i+1] = (1.0 - T0s / equivalent_stress_mode_two) / (1.0 + AParameter_mode_two); // Linear
                delamination_damage_mode_two[i+1] = 1.0 - (T0s / equivalent_stress_mode_two) * std::exp(AParameter_mode_two *
                    (1.0 - equivalent_stress_mode_two / T0s)); // Exponential

                delamination_damage_mode_two[i+1] = (delamination_damage_mode_two[i+1] >= 0.99999) ? 0.99999 : delamination_damage_mode_two[i+1];
                delamination_damage_mode_two[i+1] = (delamination_damage_mode_two[i+1] < 0.0) ? 0.0 : delamination_damage_mode_two[i+1];
            }

            // End damage calculation
        }

        for(int i=0; i < mConstitutiveLaws.size(); ++i) {
            double layer_damage_variable_mode_one = 0;
            double layer_damage_variable_mode_two = 0;

            if (delamination_damage_mode_one[i+1] > delamination_damage_mode_one[i]) {
                layer_damage_variable_mode_one = delamination_damage_mode_one[i+1];
            } else {
                layer_damage_variable_mode_one = delamination_damage_mode_one[i];
            }

            if (delamination_damage_mode_two[i+1] > delamination_damage_mode_two[i]) {
                layer_damage_variable_mode_two = delamination_damage_mode_two[i+1];
            } else {
                layer_damage_variable_mode_two = delamination_damage_mode_two[i];
            }

            layer_stress[i][2] *= ((1.0-layer_damage_variable_mode_one));
            layer_stress[i][4] *= ((1.0-layer_damage_variable_mode_one) * (1.0-layer_damage_variable_mode_two));
            layer_stress[i][5] *= ((1.0-layer_damage_variable_mode_one) * (1.0-layer_damage_variable_mode_two));
            layer_stress[i][0] *= 1.0;
            layer_stress[i][1] *= 1.0;
            layer_stress[i][3] *= 1.0;


            // if (negative_interfacial_stress_indicator[i] == false && negative_interfacial_stress_indicator[i+1] == false) {
            //     layer_stress[i][2] *= ((1.0-layer_damage_variable_mode_one));
            // }
        }
        

        for(int i=0; i < mConstitutiveLaws.size(); ++i) {
            const double factor = mCombinationFactors[i];
            delamination_damage_affected_stress_vector += factor * layer_stress[i];
        }

        auxiliar_stress_vector = delamination_damage_affected_stress_vector;
        
        noalias(rValues.GetStressVector()) = auxiliar_stress_vector;

        if (flag_const_tensor) {
            this->CalculateTangentTensor(rValues, BaseType::StressMeasure_PK2);
        }


        // Previous flags restored
        r_flags.Set(BaseType::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor);
        r_flags.Set(BaseType::COMPUTE_STRESS, flag_stress);
        r_flags.Set(BaseType::USE_ELEMENT_PROVIDED_STRAIN, flag_strain);
    }

    KRATOS_CATCH("");
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::CalculateMaterialResponseKirchhoff(ConstitutiveLaw::Parameters& rValues)
{
    CalculateMaterialResponsePK2(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void  TractionSeparationLaw3D<TDim>::CalculateMaterialResponseCauchy(ConstitutiveLaw::Parameters& rValues)
{
    CalculateMaterialResponsePK2(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::FinalizeMaterialResponsePK1(ConstitutiveLaw::Parameters& rValues)
{
    FinalizeMaterialResponsePK2(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::FinalizeMaterialResponsePK2(ConstitutiveLaw::Parameters& rValues)
{
    KRATOS_TRY;

    // Get Values to compute the constitutive law:
    Flags& r_flags = rValues.GetOptions();

    // Previous flags saved
    const bool flag_strain       = r_flags.Is(BaseType::USE_ELEMENT_PROVIDED_STRAIN);
    const bool flag_const_tensor = r_flags.Is(BaseType::COMPUTE_CONSTITUTIVE_TENSOR);
    const bool flag_stress       = r_flags.Is(BaseType::COMPUTE_STRESS);

    const Properties& r_material_properties = rValues.GetMaterialProperties();

    // The deformation gradient
    if (rValues.IsSetDeterminantF()) {
        const double determinant_f = rValues.GetDeterminantF();
        KRATOS_ERROR_IF(determinant_f < 0.0) << "Deformation gradient determinant (detF) < 0.0 : " << determinant_f << std::endl;
    }

    // All the strains must be the same, therefore we can just simply compute the strain in the first layer
    if (r_flags.IsNot(BaseType::USE_ELEMENT_PROVIDED_STRAIN)) {
        this->CalculateGreenLagrangeStrain(rValues);
        r_flags.Set(BaseType::USE_ELEMENT_PROVIDED_STRAIN, true);
    }

    // The global strain vector, constant
    const Vector strain_vector = rValues.GetStrainVector();

    if (r_flags.Is(BaseType::COMPUTE_STRESS)) {
        // Set new flags
        r_flags.Set(BaseType::COMPUTE_CONSTITUTIVE_TENSOR, false);
        r_flags.Set(BaseType::COMPUTE_STRESS, true);

        // Auxiliar stress vector
        const auto it_prop_begin       = r_material_properties.GetSubProperties().begin();
        Vector auxiliar_stress_vector  = ZeroVector(VoigtSize);
        Vector delamination_damage_affected_stress_vector  = ZeroVector(VoigtSize);
        // Vector undamaged_auxiliar_stress_vector  = ZeroVector(VoigtSize);

        // The rotation matrix
        BoundedMatrix<double, VoigtSize, VoigtSize> voigt_rotation_matrix;


        std::vector<Vector> layer_stress(mConstitutiveLaws.size());
        for (int i=0; i < mConstitutiveLaws.size(); ++i) {
            layer_stress[i].resize(6, false);
        }

        std::vector<Vector> interfacial_stress(mConstitutiveLaws.size()-1);
        for (int i=0; i < mConstitutiveLaws.size()-1; ++i) {
            interfacial_stress[i].resize(3, false);
        }

        std::vector<bool> negative_interfacial_stress_indicator(mConstitutiveLaws.size()+1);
        for (int i=0; i < mConstitutiveLaws.size()+1; ++i) {
            negative_interfacial_stress_indicator[i] = false;
        }

        for (IndexType i_layer = 0; i_layer < mConstitutiveLaws.size(); ++i_layer) {

            this->CalculateRotationMatrix(r_material_properties, voigt_rotation_matrix, i_layer);

            Properties& r_prop             = *(it_prop_begin + i_layer);
            ConstitutiveLaw::Pointer p_law = mConstitutiveLaws[i_layer];
            const double factor            = mCombinationFactors[i_layer];

            // We rotate to local axes the strain
            noalias(rValues.GetStrainVector()) = prod(voigt_rotation_matrix, strain_vector);

            rValues.SetMaterialProperties(r_prop);
            p_law->CalculateMaterialResponsePK2(rValues);

            // we return the stress and constitutive tensor to the global coordinates
            rValues.GetStressVector()        = prod(trans(voigt_rotation_matrix), rValues.GetStressVector());
            noalias(layer_stress[i_layer]) = rValues.GetStressVector();

            p_law->FinalizeMaterialResponsePK2(rValues);
            
            // we reset the properties and Strain
            rValues.SetMaterialProperties(r_material_properties);
            noalias(rValues.GetStrainVector()) = strain_vector;
        }

        const double tolerance = std::numeric_limits<double>::epsilon();
        Vector delamination_damage_mode_one(mConstitutiveLaws.size()+1);
        Vector delamination_damage_mode_two(mConstitutiveLaws.size()+1);
        Vector threshold_mode_one(mConstitutiveLaws.size()-1);
        Vector threshold_mode_two(mConstitutiveLaws.size()-1);

        noalias(delamination_damage_mode_one) = mdelamination_damage_mode_one;
        noalias(delamination_damage_mode_two) = mdelamination_damage_mode_two;
        noalias(threshold_mode_one) = mthreshold_mode_one;
        noalias(threshold_mode_two) = mthreshold_mode_two;
        
        for(int i=0; i < mConstitutiveLaws.size()-1; ++i) {

            interfacial_stress[i][0] = MacaullyBrackets((layer_stress[i][2] + layer_stress[i+1][2]) * 0.5); // interfacial normal stress
            interfacial_stress[i][1] = (layer_stress[i][4] + layer_stress[i+1][4]) * 0.5; // interfacial shear stress
            interfacial_stress[i][2] = (layer_stress[i][5] + layer_stress[i+1][5]) * 0.5; // interfacial shear stress

            const double equivalent_stress_mode_one = interfacial_stress[i][0];
            const double equivalent_stress_mode_two = std::sqrt(std::pow(interfacial_stress[i][1],2.0)+std::pow(interfacial_stress[i][2],2.0));

            if ((layer_stress[i][2] + layer_stress[i+1][2] * 0.5) < tolerance) {
                negative_interfacial_stress_indicator[i+1] = true;
            }

            // Damage calculation

            const double T0n = r_material_properties[INTERFACIAL_NORMAL_STRENGTH]; // Interfacial Normal Strength
            const double T0s = r_material_properties[INTERFACIAL_SHEAR_STRENGTH]; // Interfacial Shear Strength
            const double T0t = r_material_properties[INTERFACIAL_SHEAR_STRENGTH]; // Interfacial Shear Strength
            const double GIc = r_material_properties[MODE_ONE_FRACTURE_ENERGY]; // Mode I Energy Release Rate
            const double GIIc = r_material_properties[MODE_TWO_FRACTURE_ENERGY]; // Mode II Energy Release Rate
            const double Ei = r_material_properties[TENSILE_INTERFACE_MODULUS]; // Tensile modulus of the interface
            const double Gi = r_material_properties[SHEAR_INTERFACE_MODULUS]; // Shear modulus of the interface
            const double characteristic_length = 0.6343 * (AdvancedConstitutiveLawUtilities<VoigtSize>::CalculateCharacteristicLengthOnReferenceConfiguration(rValues.GetElementGeometry()));
        
            const double F_mode_one = equivalent_stress_mode_one - threshold_mode_one[i];
            if (F_mode_one > tolerance) {
                
                // const double AParameter_mode_one = -std::pow(T0n, 2) / (2.0 * Ei * GIc / characteristic_length); // Linear
                const double AParameter_mode_one = 1.00 / (GIc * Ei / (characteristic_length * std::pow(T0n, 2)) - 0.5); // Exponential
            
                KRATOS_ERROR_IF(AParameter_mode_one < 0.0) << "AParameter_mode_one is negative." << std::endl;

                // delamination_damage_mode_one[i+1] = (1.0 - T0n / equivalent_stress_mode_one) / (1.0 + AParameter_mode_one); // Linear
                delamination_damage_mode_one[i+1] = 1.0 - (T0n / equivalent_stress_mode_one) * std::exp(AParameter_mode_one *
                    (1.0 - equivalent_stress_mode_one / T0n)); // Exponential

                delamination_damage_mode_one[i+1] = (delamination_damage_mode_one[i+1] >= 0.99999) ? 0.99999 : delamination_damage_mode_one[i+1];
                delamination_damage_mode_one[i+1] = (delamination_damage_mode_one[i+1] < 0.0) ? 0.0 : delamination_damage_mode_one[i+1];

                mdelamination_damage_mode_one[i+1] = delamination_damage_mode_one[i+1];
                mthreshold_mode_one[i] = equivalent_stress_mode_one;

            }

            const double F_mode_two = equivalent_stress_mode_two - threshold_mode_two[i];
            if (F_mode_two > tolerance) {
                
                // const double AParameter_mode_two = -std::pow(T0s, 2) / (2.0 * Gi * GIIc / characteristic_length); // Linear
                const double AParameter_mode_two = 1.00 / (GIIc * Gi / (characteristic_length * std::pow(T0s, 2)) - 0.5); // Exponential
        
                KRATOS_ERROR_IF(AParameter_mode_two < 0.0) << "AParameter_mode_two is negative." << std::endl;

                // delamination_damage_mode_two[i+1] = (1.0 - T0s / equivalent_stress_mode_two) / (1.0 + AParameter_mode_two); // Linear
                delamination_damage_mode_two[i+1] = 1.0 - (T0s / equivalent_stress_mode_two) * std::exp(AParameter_mode_two *
                    (1.0 - equivalent_stress_mode_two / T0s)); // Exponential

                delamination_damage_mode_two[i+1] = (delamination_damage_mode_two[i+1] >= 0.99999) ? 0.99999 : delamination_damage_mode_two[i+1];
                delamination_damage_mode_two[i+1] = (delamination_damage_mode_two[i+1] < 0.0) ? 0.0 : delamination_damage_mode_two[i+1];

                mdelamination_damage_mode_two[i+1] = delamination_damage_mode_two[i+1];
                mthreshold_mode_two[i] = equivalent_stress_mode_two;
            }

            // End damage calculation
        }

        for(int i=0; i < mConstitutiveLaws.size(); ++i) {
            double layer_damage_variable_mode_one = 0;
            double layer_damage_variable_mode_two = 0;
            double maximum_damage = 0;

            if (delamination_damage_mode_one[i+1] > delamination_damage_mode_one[i]) {
                layer_damage_variable_mode_one = delamination_damage_mode_one[i+1];
            } else {
                layer_damage_variable_mode_one = delamination_damage_mode_one[i];
            }

            if (delamination_damage_mode_two[i+1] > delamination_damage_mode_two[i]) {
                layer_damage_variable_mode_two = delamination_damage_mode_two[i+1];
            } else {
                layer_damage_variable_mode_two = delamination_damage_mode_two[i];
            }

            layer_stress[i][2] *= ((1.0-layer_damage_variable_mode_one));
            layer_stress[i][4] *= ((1.0-layer_damage_variable_mode_one) * (1.0-layer_damage_variable_mode_two));
            layer_stress[i][5] *= ((1.0-layer_damage_variable_mode_one) * (1.0-layer_damage_variable_mode_two));
            layer_stress[i][0] *= 1.0;
            layer_stress[i][1] *= 1.0;
            layer_stress[i][3] *= 1.0;
        }
        

        for(int i=0; i < mConstitutiveLaws.size(); ++i) {
            const double factor = mCombinationFactors[i];
            delamination_damage_affected_stress_vector += factor * layer_stress[i];
        }

        auxiliar_stress_vector = delamination_damage_affected_stress_vector;
        
        noalias(rValues.GetStressVector()) = auxiliar_stress_vector;

        // Previous flags restored
        r_flags.Set(BaseType::COMPUTE_CONSTITUTIVE_TENSOR, flag_const_tensor);
        r_flags.Set(BaseType::COMPUTE_STRESS, flag_stress);
        r_flags.Set(BaseType::USE_ELEMENT_PROVIDED_STRAIN, flag_strain);
    }

    KRATOS_CATCH("");

    // End finalizing delamination damage
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::FinalizeMaterialResponseKirchhoff(ConstitutiveLaw::Parameters& rValues)
{
    FinalizeMaterialResponsePK2(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::FinalizeMaterialResponseCauchy(ConstitutiveLaw::Parameters& rValues)
{
    FinalizeMaterialResponsePK2(rValues);
}

/***********************************************************************************/
/***********************************************************************************/

template<unsigned int TDim>
void TractionSeparationLaw3D<TDim>::CalculateTangentTensor(ConstitutiveLaw::Parameters& rValues, const ConstitutiveLaw::StressMeasure& rStressMeasure)
{
    const Properties& r_material_properties = rValues.GetMaterialProperties();

    const bool consider_perturbation_threshold = r_material_properties.Has(CONSIDER_PERTURBATION_THRESHOLD) ? r_material_properties[CONSIDER_PERTURBATION_THRESHOLD] : true;
    const TangentOperatorEstimation tangent_operator_estimation = r_material_properties.Has(TANGENT_OPERATOR_ESTIMATION) ? static_cast<TangentOperatorEstimation>(r_material_properties[TANGENT_OPERATOR_ESTIMATION]) : TangentOperatorEstimation::SecondOrderPerturbation;

    if (tangent_operator_estimation == TangentOperatorEstimation::Analytic) {
        KRATOS_ERROR << "Analytic solution not available" << std::endl;
    } else if (tangent_operator_estimation == TangentOperatorEstimation::FirstOrderPerturbation) {
        // Calculates the Tangent Constitutive Tensor by perturbation (first order)
        TangentOperatorCalculatorUtility::CalculateTangentTensor(rValues, this, rStressMeasure, consider_perturbation_threshold, 1);
    } else if (tangent_operator_estimation == TangentOperatorEstimation::SecondOrderPerturbation) {
        // Calculates the Tangent Constitutive Tensor by perturbation (second order)
        TangentOperatorCalculatorUtility::CalculateTangentTensor(rValues, this, rStressMeasure, consider_perturbation_threshold, 2);
    } else if (tangent_operator_estimation == TangentOperatorEstimation::SecondOrderPerturbationV2) {
        // Calculates the Tangent Constitutive Tensor by perturbation (second order)
        TangentOperatorCalculatorUtility::CalculateTangentTensor(rValues, this, rStressMeasure, consider_perturbation_threshold, 4);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template class TractionSeparationLaw3D<2>;
template class TractionSeparationLaw3D<3>;

} // Namespace Kratos
