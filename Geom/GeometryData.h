#ifndef PPP_GEOMETRY_DATA_H
#define PPP_GEOMETRY_DATA_H


#include "./GeometryTypes.h"
#include "PPP/DataObject.h"
#include "PPP/Logger.h"


namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /**
     * GeometryData, base class for all geometry data mappable to vtkPolyData
     * derived from PropertyContainer(dynamic property container based on std::any)
     *
     */
    class GeometryData : public DataObject
    {

    public:
        GeometryData() = default;

    public:
        /**
         * call after Processor.prepareInput() otherwise, empty
         */
        virtual std::size_t itemCount() const final
        {
            std::size_t size = 0;

            if (contains("myShapes"))
            {
                auto shapes = getConst<ItemContainerType>("myShapes");
                size = shapes->size();
            }
            else if (contains("mySolids")) // also compared myShapeType
            {
                auto shapes = getConst<ItemContainerType>("mySolids");
                size = shapes->size();
            }
            else
            {
                LOG_F(ERROR, "not mySolids or myShapes property found, just return zero size");
            }

            return size;
        }

    }; // end of class

} // namespace Geom


#endif // header firewall