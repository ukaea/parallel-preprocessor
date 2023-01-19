#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>


/// std::any is only available in C++17,  MSVC has problem of __cplusplus value!
#if __cplusplus > 201402L || defined(_MSC_VER)
#include <any>
using std::any;
using std::any_cast;
#else
#include <boost/any.hpp>
using boost::any;
using boost::any_cast;
#endif // C++17



#define USE_BOOST_SERIALIZATION 0
#if USE_BOOST_SERIALIZATION
#include <boost/archive/text_oarchive.hpp>

///
template <class myType> void serializeAnyType(boost::archive::text_oarchive& ar, const any data)
{
    myType typedData = std::any_cast<myType>(data);
    ar << typedData;
}
typedef std::function<void(boost::archive::text_oarchive&, const any)> Serializer;
#endif

#define USE_JSON_SERIALIZATION 1
#if USE_JSON_SERIALIZATION
#include "third-party/nlohmann/json.hpp"
#endif


namespace PPP
{


#if USE_JSON_SERIALIZATION

    using json = nlohmann::json;

    typedef std::function<json(any&)> Jsonizer;
#endif

    /** \addtogroup PPP
     * @{
     */
    /// flag to control property behaviour, inspired by FreeCAD App::Property
    /// must be BitmaskType, support set by `|=`, and test by `(a&b) != 0`
    /// hex value or shift may be a better way to set enum value
    /// json can distinguish unsigned and signed integer
    /// QFlags<enum> is a macro enabled
    enum class PropertyFlag : unsigned long
    {
        None = 0, /*!< No special property type, default value */

        /// document transaction management, not used in property container
        Output = 4,       /*!< Modified property doesn't touch its parent container */
        NoRecompute = 16, /*!< Modified property doesn't touch/trigger its container for recompute */

        ///
        internal = 32,  /*!< Access control, private enforced by get<T> function */
        Immutable = 64, /*!< Property is read-only /const, can not be modified by API caller*/

        Hidden = 128,   /*!< Property won't appear in the GUI editor */
        ReadOnly = 256, /*!< Property is read-only in the GUI editor */

        // serializable,  jsonizable
        ExternalLink = 512, /*!< large data is saved externally with a link or path given */
        Transient = 1024,   /*!< tranisent or template data are not necessary to save */
    };


    /**
      a data class contains data of any type, with control flag,
      this class is used by PropertyContainer derived class only
      todo: serialization is not yet supported, static Property::TypeRegistry, to register the serializer can be a
      choice This class should be `is_standard_layout<> = true`, i.e. POD (compatible with C struct)
    */
    class Property
    {
    public:
        std::string name;
        any data;         // store only const shared_ptr of any container type, ptr can not change but content can
        std::string type; // get from demangle method, maybe used by python to create object from name

        /// enum flag: access:  public, readonly, save to file etc.
        PropertyFlag flag;

#if USE_JSON_SERIALIZATION
        // Serializer serializer;  // boost serialization
        Jsonizer jsonizer;
#endif

        /// std::string producer = "";  // which class generate this data entry
        /// function to generte this property, subscribers list
        /// timestamp MTtime;
        /// std::string doc;

    public:
        Property() = default; // default constructiable

        Property(std::string const& _name, any&& _any, std::string const& _type, Jsonizer tojson)
                : name(_name)
                , data(_any)
                , type(_type)
                , flag(PropertyFlag::None)
                , jsonizer(tojson){
                      // std::cout << name << " Property user ctor\n";
                  };

        Property(std::string const& _name, any&& _any)
                : name(_name)
                , data(_any)
                , type("")
                , flag(PropertyFlag::None){
                      // todo: try to demangle the readable type name
                      // std::cout << name << " Property user ctor\n";
                  };

        Property(Property&& a)
                : name(std::move(a.name))
                , data(std::move(a.data))
                , type(std::move(a.type))
                , flag(std::move(a.flag))
                , jsonizer(std::move(a.jsonizer)){
                      // std::cout << name << " Property's move ctor\n";
                  };

        Property& operator=(Property&& a)
        {
            name = std::move(a.name);
            type = std::move(a.type);
            data = std::move(a.data);
            flag = std::move(a.flag);
            jsonizer = std::move(a.jsonizer);
            // std::cout << name << " Property's move operator for " << data.type().name() << std::endl;
            return *this;
        }

        inline bool transient() const
        {
            return testFlag(PropertyFlag::Transient);
        }
        inline bool testFlag(PropertyFlag _flag) const
        {
            return static_cast<std::underlying_type<PropertyFlag>::type>(flag) &
                   static_cast<std::underlying_type<PropertyFlag>::type>(_flag);
        }
        /// top up (bitwise or) one specific flag
        inline void setFlag(PropertyFlag _flag)
        {
            flag = static_cast<PropertyFlag>(static_cast<std::underlying_type<PropertyFlag>::type>(flag) |
                                             static_cast<std::underlying_type<PropertyFlag>::type>(_flag));
        }
        inline void clearFlag()
        {
            flag = PropertyFlag::None;
        }

        // private: // forbidden, as construction must be done by ctor(), but PropertyContainer still need it
        Property(const Property&) = default;
        Property& operator=(const Property&);

    private:
#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        // When the class Archive corresponds to an output archive, the
        // & operator is defined similar to <<.  Likewise, when the class Archive
        // is a type of input archive the & operator is defined similar to >>.
        template <class Archive> void serialize(Archive& ar, const unsigned int version)
        {
            ar& name;
            ar& type;
            ar& flag;
        }
#endif
    };


#if USE_JSON_SERIALIZATION
    /// enable automatic data conversion between user type and json
    /// https://github.com/nlohmann/json#arbitrary-types-conversions
    /// enum can also be converted into string type, while flag into a list
    inline void to_json(json& j, const Property& p)
    {
        j = json{
            {"name", p.name}, {"type", p.type}, {"flag", p.flag}, // flag to_string()
        };
        if (p.jsonizer && (!p.transient()))
        {
            any tmp = p.data;            // a copy to convert nonconst to const type
            j["data"] = p.jsonizer(tmp); // it can be easily failed with exception
        }
        else
        {
            j["data"] = "data not serialized";
        }
    }

    /// TODO: not working, can not create object from type string in C++
    inline void from_json(const json& j, Property& p)
    {
        j.at("name").get_to(p.name);
        j.at("type").get_to(p.type);
        // j.at("data").get_to(p.data);
        j.at("flag").get_to(p.flag); // enum
    }

#endif

    /** @} */

} // namespace PPP