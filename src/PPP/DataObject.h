#pragma once

// incorporate this class into namespace by typedef
#include "../PropertyContainer/PropertyContainer.hpp"
#include "./Logger.h"
#include "./Utilities.h"


namespace PPP
{
    /// \ingroup PPP

    /**
     * \brief this framework focuses on big data (vector of items) with topology information (sparse matrix) like
     * geometry assembly
     *
     * Each item is not primitive type like tensorflow dtype: int/float, but computution intensive task
     * each DataType corresponds to a derived DataObject class
     * e.g. DataType::Geometry means each item is a TopoDS_Shape object
     * each data type should have a module (source code folder) with predefined processors
     * DataType, usually is set in Reader class where DataObject is generated
     */
    enum class AppExport DataType
    {
        Any = 0, ///< unknown data type
        Text,    ///< textual data
        Image,   ///< 2D picture, photos
        Audio,   ///< audio data like mp3, wav
        Video,   ///< movie data etc
        Geometry ///< CAD file and shapes, 3D shape
    };


    /// \ingroup PPP
    /**
     * base class for all data collection objects, mappable to vtkDataObject
     * simple and any kind of data, but must have a `dataType()` and `itemCount()`
     * if Base::Type is used, by all derived from Base::BaseObject, this is not necessary
     */
    class AppExport DataObject : public PropertyContainer
    {
    public:
        DataObject() = default;
        DataObject(DataObject&&) = default;
        DataObject(const DataObject&) = default;
        DataObject& operator=(const DataObject&) = default;
        virtual ~DataObject() = default;

        DataType dataType() const
        {
            return myDataType;
        }

        /// only use when `itemCount()` has not been overridden in derived classes
        void setItemCount(ItemIndexType count)
        {
            myItemCount = count;
        }
        /// item count that potentially can be process in parallel
        virtual ItemIndexType itemCount() const
        {
            if (myItemCount == 0)
                LOG_F(ERROR, "zero item count, try setItemCount() or override");
            return myItemCount;
        };
        /// different from tensorflow, std::vector<int64_t> shape()

    protected:
        DataType myDataType;
        ItemIndexType myItemCount = 0UL;
    };


} // namespace PPP
