
#pragma once
#include "OpenCascadeAll.h"
#include "PPP/TypeDefs.h"


/// automatically conversion from OpenCASCADE Bnd_Box to json array type
/// OpenCASCADE classes are not defined in any namespace, so must be defined here
/// they are inline functions (API not exported), therefore should not be linked
/// in order to link to these functions, split implementation into a source file
inline void to_json(nlohmann::json& j, const Bnd_Box& b)
{
    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
    b.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    j = nlohmann::json{xmin, ymin, zmin, xmax, ymax, zmax};
}

/// Bnd_Box: Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
inline void from_json(const nlohmann::json& j, Bnd_Box& b)
{
    std::vector<Standard_Real> v = j;
    b.Update(v[0], v[1], v[2], v[3], v[4], v[5]);
}

/// RGBA float array, conmponent value range [0, 1.0]
inline void to_json(nlohmann::json& j, const Quantity_Color& p)
{
    j = nlohmann::json{p.Red(), p.Green(), p.Blue(), p.Light()};
}
/// json array [R, G, B, A];   Quantity_Color c = json_array;
inline void from_json(const nlohmann::json& j, Quantity_Color& b)
{
    std::vector<Standard_Real> v = j;
    b.SetValues(v[0], v[1], v[2], Quantity_TOC_RGB);
}

namespace Geom
{
    using namespace PPP;

    /** \addtogroup Geom
     *  @{
     */

    /// each module should have a typedef ItemType
    typedef TopoDS_Shape ItemType;
    /// std::numeric_limits<Standard_Integer>::max(), is that 32bit int enough?
    typedef Standard_Integer ItemHashType;
    static const ItemHashType ItemHashMax = INT_MAX;

    typedef std::uint64_t UniqueIdType; // also define in PPP/UniqueId.h
    /// map has order (non contiguous in memory), can increase capacity
    typedef MapType<ItemHashType, TopoDS_Shape> ItemContainerType;
    typedef std::shared_ptr<ItemContainerType> ItemContainerPType;

    /**
     * from OCCT to FreeCAD style better enum name
     * integer value: ShapeType == TopAbs_ShapeEnum;  but NOT compatible
     *  */
    enum class ShapeType
    {
        Shape = TopAbs_SHAPE,
        Compound = TopAbs_COMPOUND,
        CompSolid = TopAbs_COMPSOLID,
        Solid = TopAbs_SOLID,
        Shell = TopAbs_SHELL,
        Wire = TopAbs_WIRE,
        Face = TopAbs_FACE,
        Edge = TopAbs_EDGE,
        Vertex = TopAbs_VERTEX
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(ShapeType, {
                                                {ShapeType::Shape, "Shape"},
                                                {ShapeType::Compound, "Compound"},
                                                {ShapeType::CompSolid, "CompSolid"},
                                                {ShapeType::Solid, "Solid"},
                                                {ShapeType::Shell, "Shell"},
                                                {ShapeType::Wire, "Wire"},
                                                {ShapeType::Face, "Face"},
                                                {ShapeType::Edge, "Edge"},
                                                {ShapeType::Vertex, "Vertex"},
                                            });

    /// if shape item is suppressed, an error type is kept and save as meta data
    /// some error code should be equal to CollisionType, therefore, it must be `enum class`
    enum class ShapeErrorType
    {
        NoError = 0,    ///< no error, can be used in bool check
        VolumeTooSmall, ///< volume is too smaller during shape check
        ItemInvisible,  ///< invisable/intermediate parts in CAD should not be exported, controlled by config parameter
        FloatingShape,  ///< floating shape (no contact with other) may count as error, controlled by config parameter
        BOPCheckFailed, ///< failed in BOP check, regarded as broken, need to be repaired by CAD designer
        Interference = 32,      ///< overlapping, boolean fragment gives more than 2 volumes
        Coincidence = 64,       ///< identical shapes take up the identical space, EQUAL
        Enclosure = 128,        ///< a smaller shape is embedded inside the other
        UnknownError = 256,     ///< equal to CollisionType enum value
        TessellationError = 512 ///< failed to tessellate the shape
    };
    // then CollisionInfo can be implicitly converted into json
    NLOHMANN_JSON_SERIALIZE_ENUM(ShapeErrorType, {
                                                     {ShapeErrorType::NoError, "NoError"},
                                                     {ShapeErrorType::VolumeTooSmall, "VolumeTooSmall"},
                                                     {ShapeErrorType::ItemInvisible, "ItemInvisible"},
                                                     {ShapeErrorType::BOPCheckFailed, "BOPCheckFailed"},
                                                     {ShapeErrorType::Interference, "Interference"},
                                                     {ShapeErrorType::Coincidence, "Coincidence"},
                                                     {ShapeErrorType::Enclosure, "Enclosure"},
                                                     {ShapeErrorType::UnknownError, "UnknownError"},
                                                     {ShapeErrorType::TessellationError, "TessellationError"},
                                                 });

    /// Proximity relationship between solids, faces
    /// this type can be filtered based on their value, e.g. `ctype >= Interference`
    enum CollisionType
    {
        NoCollision = 0,       ///< not related, XOR, will not affect
        Clearance = 1,         ///< within a gap threshold, may contact during deformation
        VertexContact = 2,     ///< only share vertext, it is an error in mechanal assembly
        EdgeContact = 4,       ///< share edge, but not face, it is an error in mechanal assembly
        FaceContact = 8,       ///< face contact, tangent, as mating in an assembly
        WeakInterference = 16, ///< small interference due to tolerance, can be fixed as FaceContact
        Interference = 32,     ///< overlapping, boolean fragment gives more than 2
        Coincidence = 64,      ///< identical shapes take up the identical space, EQUAL
        Enclosure = 128,       ///< a smaller shape is embedded inside the other
        Error = 256,           ///< exception during general fusion
        Unknown = 1024,        ///< volume change is big even when result count match
    };

    // const char* getCollisionTypeName(CollisionType c); // Geom.cpp has the definition

    // then CollisionInfo can be implicitly converted into json
    NLOHMANN_JSON_SERIALIZE_ENUM(CollisionType, {
                                                    {NoCollision, "NoCollision"},
                                                    {Clearance, "Clearance"},
                                                    {FaceContact, "FaceContact"},
                                                    {EdgeContact, "EdgeContact"},
                                                    {VertexContact, "VertexContact"},
                                                    {Interference, "Interference"},
                                                    {Coincidence, "Coincidence"},
                                                    {Enclosure, "Enclosure"},
                                                    {Error, "Error"},
                                                    {Unknown, "Unknown"},
                                                });
    /// for CollisionType::Interference >= 32, conversion is meaningful
    // function in header file must de declared as inline
    inline ShapeErrorType to_ShapeErrorType(const CollisionType ctype)
    {
        int i = static_cast<int>(ctype);
        if (i >= CollisionType::Interference and i <= CollisionType::Error)
            return static_cast<ShapeErrorType>(ctype);
        else
            return ShapeErrorType::NoError;
    }

    struct CollisionInfo
    {
    public:
        ItemIndexType first;
        ItemIndexType second;
        double value; /// tolerance, or distance
        CollisionType type;

        CollisionInfo() = default;
        // C++20 prevents conversion form <brace-enclosed initializer list> to this type
        CollisionInfo(ItemIndexType _first, ItemIndexType _second, double _value, CollisionType _type)
                : first(_first)
                , second(_second)
                , value(_value)
                , type(_type)
        {
        }
    };
    inline void to_json(json& j, const CollisionInfo& p)
    {
        j = json{{"firstIndex", p.first}, {"secondIndex", p.second}, {"value", p.value}, {"collisionType", p.type}};
    }


    /**
     * AP214 has only material name and density value, no other information
     * XCAFDoc_Material,  it is unclear densityValueType means in XCAFDoc_Material
     */
    class Material
    {
    public:
        std::string name;
        double density;
        std::string description;
        Material() = default;

        Material(std::string _name, double _density = 0.0)
                : name(_name)
                , density(_density)
        {
        }
    };

    inline void to_json(json& j, const Material& p)
    {
        // j = json{{"name", p.name}, {"density", p.density}};
        j = p.name; // tmp change the output to single string type
    }

    /// must be defined in the same namespace as Material class
    inline void from_json(const json& j, Material& p)
    {
        if (j.contains("name"))
        {
            j.at("name").get_to(p.name);
            j.at("density").get_to(p.density);
        }
        else // material json is just a string type
        {
            j.get_to(p.name);
            p.density = 0.0;
        }
    }

    /**
     * data class which can be saved into json file, bounbox is not included
     * boundingBox.Get(xmin,ymin,zmin, xmax,ymax,zmax);
     */
    class GeometryProperty
    {
    public:
        double volume;
        double area;
        double perimeter;
        double tolerance;
        int solidCount;
        int faceCount;
        int edgeCount;
        std::vector<double> centerOfMass;

    public:
        GeometryProperty() = default;
        // GeometryProperty(GeometryProperty&& p)
    };

    /// enable automatic data conversion between user type and json
    /// https://github.com/nlohmann/json#arbitrary-types-conversions
    /// from_json() may not safe, use with caution!
    inline void to_json(json& j, const GeometryProperty& p)
    {
        j = json{{"volume", p.volume},         {"area", p.area},
                 {"perimeter", p.perimeter},   {"tolerance", p.tolerance},
                 {"solidCount", p.solidCount}, {"faceCount", p.faceCount},
                 {"edgeCount", p.edgeCount},   {"centerOfMass", p.centerOfMass}};
    }

    inline void from_json(const json& j, GeometryProperty& p)
    {
        j.at("volume").get_to(p.volume);
        j.at("area").get_to(p.area);
        j.at("perimeter").get_to(p.perimeter);
        j.at("tolerance").get_to(p.tolerance);
        j.at("solidCount").get_to(p.solidCount);
        j.at("faceCount").get_to(p.faceCount);
        j.at("edgeCount").get_to(p.edgeCount);
        j.at("centerOfMass").get_to(p.centerOfMass);
    }
    /** @}*/
} // namespace Geom