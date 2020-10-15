
#include <fstream>
#include <set>
#include <string>
#include <unordered_map>

#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>


// copy 3 header files to make it works
#include "../third-party/nlohmann/json.hpp"
#include "PPP/UniqueId.h" // this file also dep "third-party/half.hpp"

/// shorten the name for convenience
using json = nlohmann::json;

auto calcUniqueId(const TopoDS_Shape& s)
{
    GProp_GProps v_props;
    BRepGProp::VolumeProperties(s, v_props);

    auto gid = PPP::Utilities::geometryUniqueId(
        v_props.Mass(), {v_props.CentreOfMass().X(), v_props.CentreOfMass().Y(), v_props.CentreOfMass().Z()});
    return gid;
}

// gpMap is what needed to get all information
void validateUniqueId(const std::string filename, const std::string jsonFile)
{
    // read json file and build a id->property map
    std::unordered_map<uint64_t, json> gpMap;
    std::set<uint64_t> idSet;

    std::ifstream inf(jsonFile);
    if (inf.fail())
        throw std::runtime_error("failed to read json file " + jsonFile);
    json j = json::parse(inf);

    for (const auto& p : j)
    {
        gpMap.insert_or_assign(p["uniqueId"].get<uint64_t>(), p);
        idSet.insert(p["uniqueId"].get<uint64_t>());
    }

    BRep_Builder cBuilder;
    TopoDS_Shape shape;
    BRepTools::Read(shape, filename.c_str(), cBuilder);
    if (shape.IsNull())
        throw std::runtime_error("brep read data is Null for:" + filename);

    // explore solids and calc Id
    int solidCount = 0;
    TopExp_Explorer Ex(shape, TopAbs_SOLID);
    while (Ex.More())
    {
        const TopoDS_Shape& s = Ex.Current();
        auto gid = calcUniqueId(s);
        if (gpMap.find(gid) == gpMap.end())
        {
            std::cout << "Id not found in metadata json file" << gid << std::endl;
        }
        else
        {
            const auto& d = gpMap[gid];
            if (d["sequenceID"] != solidCount)
                std::cout << "written sequenceID " << d["sequenceID"]
                          << " does not equal to read backed by TopExp_Explorer " << solidCount << std::endl;
            idSet.erase(gid); // for debugging, to see unmatched Id
        }
        solidCount++; // sequenceID, started with zero
        Ex.Next();
    }
    if (idSet.size())
    {
        std::cout << "Error: There is unmatched geometry Id";
    }
    else
    {
        std::cout << "Done: Each geometry Id has matched in meta data";
    }
}

// assuming geometry and json filename share the same stem exception suffix
void validateUniqueId(std::string geometry_filename)
{
    std::string metadata_suffix = "_metadata.json";
    std::string geometry_suffix = ".brep";
    std::string jsonFile =
        geometry_filename.replace(geometry_filename.find(geometry_suffix), geometry_suffix.length(), metadata_suffix);

    validateUniqueId(geometry_filename, jsonFile);
}

int main(int argc, char* argv[])
{
    if (argc > 2)
    {
        std::cout << argv[1] << std::endl << argv[2] << std::endl;
        validateUniqueId(argv[1], argv[2]);
    }
    else if (argc == 2)
    {
        validateUniqueId(argv[1]);
    }
    else
    {
        std::cout << "Usage: thisProgram geomfilename jsonfilename" << std::endl;
    }
    return 0;
}