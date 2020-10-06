#pragma once
#include "TypeDefs.h"

namespace PPP
{
    /// \ingroup PPP
    /**
     * user input configuration parameter to control the process behaviour
     * it is a data class, mappable to a json object for IO.
     * type, value are compulsory fields, other are optional fields.
     * template parameter T: only json supported C++ type is supported, string, bool, double, integer
     *
     * */
    template <typename T> class Parameter
    {
    public:
        T value;          /// enum is string, if there is enum to json conversion exists, it can be converted
        std::string type; /// datatype: path, double, integer, string, enum, bool
        /// those field below are optional
        std::string name;     /// this can be actually name of the json object in config
        std::string unit;     /// for physical quantities only
        std::vector<T> range; /// lower and upper limit for numeric, a set for enum, string types
        std::string doc;


        static Parameter fromJson(const json& j)
        {
            Parameter p;
            p.type = j["type"].get<std::string>();
            p.value = j["value"].get<T>();
            if (j.contains("name"))
                p.name = j["name"].get<std::string>();
            if (j.contains("doc"))
                p.doc = j["doc"].get<std::string>();
            if (j.contains("unit"))
                p.unit = j["unit"].get<std::string>();
            if (j.contains("range"))
            {
                for (const auto& je : j["range"])
                {
                    p.range.push_back(je.get<T>()); // support enum as json string
                }
                // p.range = j["range"].get<std::vector<T>>();  no support for enum
            }
            return p;
        }

        json toJson() const
        {
            const Parameter& p = *this;
            return json{{"type", p.type},   {"name", p.name}, {"value", p.value},
                        {"range", p.range}, {"unit", p.unit}, {"doc", p.doc}};
        }
    };

#if defined(_WIN32)
    template class __declspec(dllexport) Parameter<double>;
    // template class __declspec(dllexport) Parameter<float>;
    template class __declspec(dllexport) Parameter<bool>;
    template class __declspec(dllexport) Parameter<int64_t>;
    template class __declspec(dllexport) Parameter<std::string>;
#endif


} // namespace PPP