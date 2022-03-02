
#ifndef PPP_UTILITIES_H
#define PPP_UTILITIES_H

#include "PreCompiled.h"
#include "TypeDefs.h"


namespace PPP
{

    /** utility methods for App module */
    namespace Utilities
    {

        int AppExport runCommand(std::string cmd, std::string output_file = "");

        /// timeit::execute<F, Args>(f, arg);
        template <typename TimeT = std::chrono::milliseconds> struct timeit
        {
            template <typename F, typename... Args> static typename TimeT::rep execute(F&& func, Args&&... args)
            {
                auto start = std::chrono::steady_clock::now();
                std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
                auto duration = std::chrono::duration_cast<TimeT>(std::chrono::steady_clock::now() - start);
                return duration.count();
            }
        };

        /** A more flexible version of file extension check
         * C++17 `std::filesystem::path::extension` counts `.txt` as extension
         * this function return true for `txt`, `.txt` and `.TXT` case-insensitively
         * */
        bool AppExport hasFileExt(const std::string& filepath, const std::string& extension);

        /// read all lines a text file into the output vector (the second parameter)
        bool AppExport getFileContent(const std::string fileName, std::vector<std::string>& vecOfStrs);

        /// user must make sure data can be implicitly converted into json object
        template <class T> bool toJson(const T& data, const std::string& filename);

        /// todo: uniqueness is not guatanteed, the return type is size_t as std::hash
        template <typename ValueType> std::size_t hashVector(const std::vector<ValueType> values);

        std::string AppExport timeStamp();
        std::string AppExport timeStampFileName(const std::string filename);


    } // namespace Utilities

} // namespace PPP


#if !defined(_MSC_VER) && __cplusplus < 201402L
/// make `make_unique()` in C++14 available for C++11
/// solution from
/// <https://stackoverflow.com/questions/17902405/how-to-implement-make-unique-function-in-c11>
namespace std
{
    template <class T> struct _Unique_if
    {
        typedef unique_ptr<T> _Single_object;
    };

    template <class T> struct _Unique_if<T[]>
    {
        typedef unique_ptr<T[]> _Unknown_bound;
    };

    template <class T, size_t N> struct _Unique_if<T[N]>
    {
        typedef void _Known_bound;
    };

    template <class T, class... Args> typename _Unique_if<T>::_Single_object make_unique(Args&&... args)
    {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    template <class T> typename _Unique_if<T>::_Unknown_bound make_unique(size_t n)
    {
        typedef typename remove_extent<T>::type U;
        return unique_ptr<T>(new U[n]());
    }

    template <class T, class... Args> typename _Unique_if<T>::_Known_bound make_unique(Args&&...) = delete;
} // namespace std
#endif

#endif // header firewall