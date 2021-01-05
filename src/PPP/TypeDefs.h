#pragma once

#include <array>
#include <iso646.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../third-party/nlohmann/json.hpp"
#include "fs.h"

/// GCC 9 is needed to use magic enum, is_magic_enum_supported or macro MAGIC_ENUM_SUPPORTED
/// MSVC Compile error: fatal error C1026: parser stack overflow, program too complex  for MAGIC_ENUM_RANGE_MAX 2048
#define MAGIC_ENUM_RANGE_MAX 512 // must be less than INT16_MAX
#include <magic_enum.hpp>


namespace PPP
{
    /** \addtogroup PPP
     * @{
     */
    /// shorten the name for convenience
    using json = nlohmann::json;

    /** input configuration, currently it is a `typedef` of json, in the future, make it a proper class
     * mappable to `vtkInformation` as input
     *  */
    using Config = nlohmann::json;

    /** input configuration, currently it is a `typedef` of json, in the future, make it a proper class
     * mappable to `vtkInformation` as output
     * */
    using Information = nlohmann::json;

    /// introduce enum_cast enum_name into this namespace
    using namespace magic_enum;

    /**
     * Processing on CPU should be implemented for each processor class, for debugging purpose.
     * GPU is not supported yet (tensorflow etc can do it), possibly using OpenMP offloaded to GPU.
     * currently, only CPU is implemented.
     * */
    enum class DevicePreference
    {
        CPU,  ///< item count is small, but processing each item take time and complext to push to GPU
        GPU,  ///< GPU acceleraton, suitable for large item-count but simple calculation
        FPGA, ///< good for realtime data processing, by hardware parallization
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(DevicePreference, {
                                                       {DevicePreference::CPU, "CPU"},
                                                       {DevicePreference::GPU, "GPU"},
                                                       {DevicePreference::FPGA, "FPGA"},
                                                   });

    /**
     * data are organized as 1D array of items, each item is a piece of data segment,
     * i.e. one song, one textual file, one image, to be processed in a thread
     * currently a template alias of std::vector<>, which is contiguous in memory.
     *  it may be replaced by another type for MPI parallel
     * distributive data structure/container with a different memory allocator
     * note: propertyContainer's any_cast consider template alias are different type
     * so be consistent using VectorType and MapType
     * */
    template <class Item> using VectorType = std::vector<Item>;

    /**
     * Sparse vector, based on shared_ptr, initialize value and set size before usage
     * e.g. `mySpVec<std::vector<double>>`
     * */
    template <class Item> using SparseVector = std::vector<std::shared_ptr<Item>>;

    /**
     * MapType choice for parallel preprocessor
     * std::set or std::map, based on binary tree, has time complexity O(logN) for find and insert
     * while unordered_map and unorderred_set, based on hash, can reserve space (crucial).
     * time complexity O(1) or O(N), depends on hash function performance.
     * */
    template <class Key, class Value> using MapType = std::unordered_map<Key, Value>;


    /**
     * data vector/array/list index,  widely used in STL container
     * std::size_t is 64bit unsigned integer, would be sufficient for big data
     */
    typedef std::size_t ItemIndexType;

    /// Item traverse pattern, to balance work load on each worker/thread
    /// used with `myCharacteristics["indexDimension"]` default to 1.
    enum class IndexPattern
    {
        Linear,            ///< each item has the same computation time, sliced into block, not coupled
        FilteredVector,    ///< not each item will be ProcessItem(), executor check "required()"
        PartitionIdVector, ///< item has an partition Id == threadID to bind with a specific thread
        /// 2 operands operation not coupled operaton
        UpperTriangle, ///< for itemPair,  op(i, j) for i<j, skip diagonal op(i,i)
        LowerTriangle, ///< not in use, as executor use UpperTriangle by default
        DenseMatrix,   ///< 2D traverse, both op(i, j), op(j, i)
        /// coupled operation
        SparseMatrix,   ///< must provide a indexingMatrix/ has a couplingMatrix field
        FilteredMatrix, ///< executor will calc the sparse index matrix by `isCoupledPair()`

    };

    NLOHMANN_JSON_SERIALIZE_ENUM(IndexPattern, {
                                                   {IndexPattern::Linear, "Linear"},
                                                   {IndexPattern::UpperTriangle, "UpperTriangle"},
                                                   {IndexPattern::LowerTriangle, "LowerTriangle"},
                                                   {IndexPattern::DenseMatrix, "DenseMatrix"},
                                                   {IndexPattern::SparseMatrix, "SparseMatrix"},
                                                   {IndexPattern::FilteredMatrix, "FilteredMatrix"},
                                                   {IndexPattern::PartitionIdVector, "PartitionIdVector"},
                                               });
    /** @}*/

} // namespace PPP
