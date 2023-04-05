//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   license: OptimizationApplication/license.txt
//
//  Main author:     Suneth Warnakulasuriya
//

#pragma once

// System includes
#include <string>
#include <vector>
#include <variant>

// Project includes
#include "includes/define.h"
#include "includes/model_part.h"

// Application includes
#include "container_variable_data_holder.h"

namespace Kratos {

///@name Kratos Classes
///@{
/**
 * @brief Construct a new CollectiveVariableDataHolder instance
 * @details This constructs a CollectiveVariableHolderInstance which can hold list of ContainerExpresions of different types.
 *          The list within the instance can be treated as a single value, therefore all the binary operations present in the ContainerExpressions
 *          are also available with this container. This uses std::variant to store different types of containers. Since these containers memeory footprint
 *          is the same, the memory footprint of the std::variant will be almost equal to the memory footprint of a single container for all the container types.
 *
 */
class KRATOS_API(OPTIMIZATION_APPLICATION) CollectiveVariableDataHolder {
public:
    ///@name Type definitions
    ///@{

    using IndexType = std::size_t;

    using HistoricalContainerVariableDataHolderPointerType =  ContainerVariableDataHolder<ModelPart::NodesContainerType, HistoricalContainerDataIO>::Pointer;

    using NodalContainerVariableDataHolderPointerType =  ContainerVariableDataHolder<ModelPart::NodesContainerType, NonHistoricalContainerDataIO>::Pointer;

    using ConditionContainerVariableDataHolderPointerType =  ContainerVariableDataHolder<ModelPart::ConditionsContainerType, NonHistoricalContainerDataIO>::Pointer;

    using ElementContainerVariableDataHolderPointerType =  ContainerVariableDataHolder<ModelPart::ElementsContainerType, NonHistoricalContainerDataIO>::Pointer;

    using ConditionPropertiesContainerVariableDataHolderPointerType =  ContainerVariableDataHolder<ModelPart::ConditionsContainerType, PropertiesContainerDataIO>::Pointer;

    using ElementPropertiesContainerVariableDataHolderPointerType =  ContainerVariableDataHolder<ModelPart::ElementsContainerType, PropertiesContainerDataIO>::Pointer;

    using ContainerVariableDataHolderPointerVariantType = std::variant<
            HistoricalContainerVariableDataHolderPointerType,
            NodalContainerVariableDataHolderPointerType,
            ConditionContainerVariableDataHolderPointerType,
            ElementContainerVariableDataHolderPointerType,
            ConditionPropertiesContainerVariableDataHolderPointerType,
            ElementPropertiesContainerVariableDataHolderPointerType>;

    KRATOS_CLASS_POINTER_DEFINITION(CollectiveVariableDataHolder);

    ///@}
    ///@name Life cycle
    ///#{

    // Default constructor
    CollectiveVariableDataHolder() noexcept = default;


    // Constructor with list
    CollectiveVariableDataHolder(const std::vector<ContainerVariableDataHolderPointerVariantType>& rContainerVariableDataHolderPointersList);

    // Copy consttructor
    CollectiveVariableDataHolder(const CollectiveVariableDataHolder& rOther);

    // destructor
    ~CollectiveVariableDataHolder() = default;

    ///@}
    ///@name Public operations
    ///@{

    CollectiveVariableDataHolder Clone() const;

    CollectiveVariableDataHolder CloneWithDataInitializedToZero()  const;

    void AddVariableDataHolder(const ContainerVariableDataHolderPointerVariantType& pVariableDataHolder);

    void AddCollectiveVariableDataHolder(const CollectiveVariableDataHolder& rCollectiveVariableDataHolder);

    void ClearVariableDataHolders();

    std::vector<ContainerVariableDataHolderPointerVariantType> GetVariableDataHolders();

    std::vector<ContainerVariableDataHolderPointerVariantType> GetVariableDataHolders() const;

    bool IsCompatibleWith(const CollectiveVariableDataHolder& rOther) const;

    ///@}
    ///@name Public operators
    ///@{

    CollectiveVariableDataHolder operator+(const CollectiveVariableDataHolder& rOther) const;

    CollectiveVariableDataHolder& operator+=(const CollectiveVariableDataHolder& rOther);

    CollectiveVariableDataHolder operator+(const double Value) const;

    CollectiveVariableDataHolder& operator+=(const double Value);

    CollectiveVariableDataHolder operator-(const CollectiveVariableDataHolder& rOther) const;

    CollectiveVariableDataHolder& operator-=(const CollectiveVariableDataHolder& rOther);

    CollectiveVariableDataHolder operator-(const double Value) const;

    CollectiveVariableDataHolder& operator-=(const double Value);

    CollectiveVariableDataHolder operator*(const CollectiveVariableDataHolder& rOther) const;

    CollectiveVariableDataHolder& operator*=(const CollectiveVariableDataHolder& rOther);

    CollectiveVariableDataHolder operator*(const double Value) const;

    CollectiveVariableDataHolder& operator*=(const double Value);

    CollectiveVariableDataHolder operator/(const CollectiveVariableDataHolder& rOther) const;

    CollectiveVariableDataHolder& operator/=(const CollectiveVariableDataHolder& rOther);

    CollectiveVariableDataHolder operator/(const double Value) const;

    CollectiveVariableDataHolder& operator/=(const double Value);

    CollectiveVariableDataHolder operator^(const CollectiveVariableDataHolder& rOther) const;

    CollectiveVariableDataHolder& operator^=(const CollectiveVariableDataHolder& rOther);

    CollectiveVariableDataHolder operator^(const double Value) const;

    CollectiveVariableDataHolder& operator^=(const double Value);

    ///@}
    ///@name Input and output
    ///@{

    std::string Info() const;

    ///@}
private:
    ///@name Private variables
    ///@{

    std::vector<ContainerVariableDataHolderPointerVariantType> mContainerVariableDataHolderPointersList;

    ///@}
};

///@}
///@name Input and output
///@{

/// output stream function
inline std::ostream& operator<<(
    std::ostream& rOStream,
    const CollectiveVariableDataHolder& rThis)
{
    return rOStream << rThis.Info();
}

///@}

} // namespace Kratos