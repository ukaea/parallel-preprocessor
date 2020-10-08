#pragma once

#include <iomanip>
#include <string>

/// using TBB current hash map for thread-safety
#if USE_CONCURRENT_HASH_CONTAINER
#include <tbb/concurrent_hash_map.h>
template <class Key, class Value> using HashContainer = tbb::concurrent_hash_map<Key, Value>;
#else
#include <map>
#include <unordered_map>
template <class Key, class Value> using HashContainer = std::unordered_map<Key, Value>;
#endif

/// macro options
#if defined(_MSC_VER)
#define USE_BOOST_DEMANGLE 1
#else
#define USE_BOOST_DEMANGLE 0
#endif

#if USE_BOOST_DEMANGLE
#include <boost/core/demangle.hpp>
using boost::core::demangle;
#else

/// either inline or impl in a source file `demangle.cpp` and added to build system
#include <cxxabi.h>
inline std::string demangle(char const* mangledName)
{
    int status;
    char* realname;
    realname = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
    std::string name;
    if (status == 0)
    {
        name = std::string(realname);
        free(realname);
    }
    if (status == -2)
    {
        // throw mangled_name is not a valid name under the C++ ABI mangling rules";
    }
    return name;
}
#endif

#include "./Property.h"

namespace PPP
{

    template <typename myType> json anyToJson(any& a)
    {
        std::cout << "saved any type: " << std::string(demangle(a.type().name())) << std::endl;
        /// NOTE: property data are saved as std::shared_ptr<T>
        std::shared_ptr<myType> pd = any_cast<std::shared_ptr<myType>>(a);
        std::cout << "data type without pointer: " << std::string(demangle(typeid(*pd).name())) << std::endl;
        json j = *pd; /// auto conversion, if `to_json()` has been defined in nlohmann json way
        return j;
    }

    /// \ingroup PPP
    /**
     * dynamic heterogenous container to simulate dynamic property, based on std::any
     *
     * APIs  are compatible with STL associative container with STL iterator, but
     * those APIs protected because Property type is only accessible by derived class
     * Public access can only access pointer or value to the Property.data of specific type.
     *
     * Data saved into std::any are `shared_ptr<T>`, to avoid large data copying around
     */
    class PropertyContainer
    {
    protected:
        HashContainer<std::string, Property> myProperties;

        typedef std::string key_type;
        typedef HashContainer<std::string, Property>::const_iterator const_iterator_type;

        /// smart pointer type is manage property data, it could be changed to unique_ptr<>
        /// in case of changing, all `std::make_shared<>` must be replaced with `std::make_unique<>()`
        template <typename T> using default_pointer_type = std::shared_ptr<T>;
        /// path can be filepath (std::string), or HDF5 path, etc, @sa DataStorage class
        typedef std::string DataPath;

    protected:
        /**  enforced move semantics, this container must have the ownership of property*/
        void addProperty(const Property&& p)
        {
            myProperties.emplace(std::string(p.name), p);
        }
        /// return const reference
        const Property& getProperty(const std::string& key) const
        {
            if (contains(key))
            {
                return myProperties.at(key);
            }
            else
            {
                throw std::runtime_error("key not found in this container");
            }
        }
        /// return by reference
        Property& getProperty(const std::string& key)
        {
            if (contains(key))
            {
                return myProperties[key];
            }
            else
            {
                throw std::runtime_error("key not found in this container");
            }
        }

    public:
        PropertyContainer() = default;
        virtual ~PropertyContainer() = default;

        /// NOTE: copy constructor will be disabled once move constructor is declared

        PropertyContainer(PropertyContainer&& pc)
                : myProperties(std::move(pc.myProperties))
        {
        }

        /// @{
        /** get property data by key name, return a `std::shared_ptr<T>` to the property data
         */
        template <typename T> default_pointer_type<T> get(const std::string& key)
        {
            if (contains(key))
            {
                any an = myProperties[key].data;
                return any_cast<default_pointer_type<T>>(an);
            }
            else
            {
                throw std::runtime_error("no property named as: " + key);
            }
        }

        /// get the data also erase it from the container
        template <typename T, typename Ptr = default_pointer_type<T>> Ptr extract(const std::string& key)
        {
            if (contains(key))
            {
                any an = myProperties[key].data;
                myProperties.erase(key);
                return any_cast<default_pointer_type<T>>(an);
            }
            else
            {
                throw std::runtime_error("no property named as: " + key);
            }
        }

        /** const version of get property data, return `std::shared_ptr<const T>`
         *  usage: `get<const std::string>()`, this works just like `getConst<std::string>()`
         * */
        template <typename T, typename = std::enable_if<std::is_const<T>::value>>
        default_pointer_type<T> get(const std::string& key) const
        {
            std::cout << "DEBUG info: const versoin of get<const AType>() is called!" << std::endl;
            if (contains(key))
            {
                return any_cast<default_pointer_type<T>>(myProperties.at(key).data);
            }
            else
            {
                throw std::runtime_error("no property named as: " + key);
            }
        }

        /** const version of get property data, return the value type
         * */
        template <typename T> T getValue(const std::string& key) const
        {
            std::cout << "DEBUG info: const versoin of get<const AType>() is called!" << std::endl;
            if (contains(key))
            {
                auto ptr = any_cast<default_pointer_type<const T>>(myProperties.at(key).data);
                return T(*ptr);
            }
            else
            {
                throw std::runtime_error("no property named as: " + key);
            }
        }

        /** const version of get property data, return `std::shared_ptr<const T>`
         * */
        template <typename T> default_pointer_type<const T> getConst(const std::string& key) const
        {
            if (contains(key))
            {
                return any_cast<default_pointer_type<T>>(myProperties.at(key).data);
            }
            else
            {
                throw std::runtime_error("no property named as: " + key);
            }
        }

        /// set value by move semantics, like std::map emplace(), it accepts only rvalue type,
        /// produced by std::move(your_type)
        template <typename T> void emplace(const std::string& key, T&& data)
        {
            set<T>(key, std::make_shared<T>(std::move(data)));
        }

        /* set value by move semantics, given right reference to the data object
         * it is enforced by type trait at compiling time,
         * by `std::enable_if<std::is_rvalue_reference<DType>::value>>`
         */
        template <class DType, typename = std::enable_if<std::is_rvalue_reference<DType>::value>>
        void set(const std::string& key, DType data)
        {
            std::cout << "DEBUG info: set() with std::enable_if<std::is_rvalue_reference<T>::value>" << std::endl;
            typename std::remove_reference<DType>::type realType;
            set<decltype(realType)>(key, std::make_shared<decltype(realType)>(std::move(data)));
        }

        /** overloaded template function by providing const left reference or arithmetic value
         */
        template <typename T> void setValue(const std::string& key, const T& data)
        {
            set<T>(key, std::make_shared<T>(data));
        }


        /** data must be the smart pointer type: `set<T>("key_name", std::make_shared<T>())`,
         * NOTE: do not make a shared_ptr from local object (auto deleted if out of scope),
         * by default data is NOT serializable
         */
        template <typename T, typename Ptr = default_pointer_type<T>> void set(const std::string& key, Ptr pdata)
        {
            if (contains(key))
            {
                myProperties.erase(key);
            }
            auto typeName = demangle(typeid(*pdata).name()); // from pointer type to object type
            Property p(key, any(pdata), typeName, nullptr);
            p.setFlag(PropertyFlag::Transient); // this name may be not good enough, subjective to change
            myProperties.emplace(key, std::move(p));
        }

        /** accept only rvalue, produced by std::move(your_type)
         */
        template <class T, typename = std::enable_if<std::is_rvalue_reference<T>::value>>
        void setSerializable(const std::string& key, T data, Jsonizer _jsonizer = nullptr)
        {
            std::cout << "set() with std::enable_if<std::is_rvalue_reference<T>::value>" << std::endl;
            typename std::remove_reference<T>::type realType;
            Jsonizer jsonizer;
            if (!_jsonizer)
                jsonizer = anyToJson<decltype(realType)>;
            else
                jsonizer = _jsonizer;
            setSerializable<decltype(realType)>(key, std::make_shared<decltype(realType)>(std::move(data)), jsonizer);
        }

        /**
         * \brief set property data using default property flags and a default json serializer
         *
         * if user wants this container to own/manage the life cycle of the data object,
         *  using `setData<>` or `set(key, std::make_shared<T>(std::move(your_data_value_type)))`
         *
         * @param Ptr template parameter pointer type, default to std::shared_ptr<T>
         *       - std::shared_ptr<T>:
         *       - std::decltype<T>: as the data type it is (value type semantics), not tested
         *       - std::unique_ptr<T>: emplace and extract movable data
         * @param key `std::string` or `const *char` which implicitly converts into `std::string`
         *       if the key have existed, just replace the key and issue a warning log
         */
        template <typename T, typename Ptr = default_pointer_type<T>>
        void setSerializable(const std::string& key, Ptr pdata, Jsonizer jsonizer = anyToJson<T>)
        {
            if (contains(key))
            {
                myProperties.erase(key);
            }
            auto typeName = demangle(typeid(*pdata).name()); // from pointer type to object type
            Property p(key, any(pdata), typeName, jsonizer);
            myProperties.emplace(key, std::move(p));
        }

        /// if the key existed, erase first
        template <typename T, typename Ptr = default_pointer_type<T>>
        void setSerializable(const std::string& key, Ptr pdata, const DataPath& _path)
        {
            if (contains(key))
            {
                myProperties.erase(key);
            }
            auto typeName = demangle(typeid(*pdata).name()); // from pointer type to object type

            Jsonizer jsonizer = [&_path](any&) { return _path; };
            Property p(key, any(pdata), typeName, jsonizer);
            if (_path.size())
                p.setFlag(PropertyFlag::ExternalLink); // this name may be not good enough, subjective to change
            else
                p.setFlag(PropertyFlag::Transient); // this name may be not good enough, subjective to change
            myProperties.emplace(key, std::move(p));
        }


        /// @}

        /// @{ STL container compatible API

        inline std::size_t size() const
        {
            return myProperties.size();
        }

        inline bool empty() const
        {
            return myProperties.empty();
        }

        /** bring C++20 STL to this class */
        inline bool contains(const std::string& key) const
        {
            return myProperties.find(key) != myProperties.end();
        }
        /** no effect if no such key  */
        inline void erase(const std::string& key) noexcept
        {
            myProperties.erase(key);
        }

        void save(const std::string filename)
        {
            std::ofstream jf(filename);
            jf << std::setw(4);
            json j;
            for (const auto& item : myProperties)
            {
                /// NOTE: skip if(item.second.transient())
                j[item.first] = item.second;
            }
            jf << j;
        }
        /// @}

        /// @{
        /// property flag check
        inline bool readOnly(const std::string&)
        {
            return false;
        }

        inline bool internal(const std::string&)
        {
            return false;
        }

        inline bool transient(const std::string&)
        {
            return false;
        }
        /// @}
    };

} // namespace PPP