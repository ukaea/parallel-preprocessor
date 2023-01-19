#include "Utilities.h"


namespace PPP
{
    namespace Utilities
    {

        int runCommand(const std::string cmd, const std::string output_file)
        {
            std::string cmdline = cmd;
            if (output_file.size() > 4)
                cmdline = cmd + " > " + output_file;
            return std::system(cmdline.c_str());
        }

        bool hasFileExt(const std::string& filepath, const std::string& extension)
        {
            std::string ext = extension;
            if (extension.find('.') == 0)
                ext = extension.substr(1, extension.length() - 1);

            auto i = filepath.rfind('.', filepath.length());
            if (i != std::string::npos)
            {
                auto extension = filepath.substr(i + 1, filepath.length() - i);
                // std::cout << "file extension extracted: " << extension << std::endl;
                if (extension.length() && extension.length() == ext.length())
                {
                    for (size_t j = 0; j < extension.length(); j++)
                    {
                        if (std::tolower(ext[j]) != std::tolower(extension[j]))
                        {
                            return false;
                        }
                    }
                    return true;
                }
            }
            return false;
        }

        /// read all lines a text file into the output vector (the second parameter)
        bool getFileContent(const std::string fileName, std::vector<std::string>& vecOfStrs)
        {
            // Open the File
            std::ifstream in(fileName.c_str());

            // Check if object is valid
            if (!in)
            {
                std::cerr << "Cannot open the File : " << fileName << std::endl;
                return false;
            }

            std::string str;
            // Read the next line from File until it reaches the end.
            while (std::getline(in, str))
            {
                // Line contains string of length > 0 then save it in vector
                if (str.size() > 0)
                    vecOfStrs.push_back(str);
            }
            // Close The File
            in.close();
            return true;
        }

        /// user must make sure data can be implicitly converted into json object
        template <class T> bool toJson(const T& data, const std::string& filename)
        {
            json jObj = data; // implicitly converted into json object
            std::ofstream of(filename, std::ios::out | std::ios::binary);
            std::vector<std::uint8_t> v_buffer;
            if (hasFileExt(filename, "json"))
            {
                of << jObj;
            }
            else if (hasFileExt(filename, "msgpack"))
            {
                v_buffer = json::to_msgpack(jObj);
            }
            else if (hasFileExt(filename, "ubjson"))
            {
                v_buffer = json::to_ubjson(jObj);
            }
            else if (hasFileExt(filename, "cbor"))
            {
                v_buffer = json::to_cbor(jObj);
            }
            else if (hasFileExt(filename, "bson"))
            {
                v_buffer = json::to_bson(jObj);
            }
            else
            {
                throw std::runtime_error(
                    "format is guessed from file suffix, only `.ubjson, .msgpack, .cbor, .bson` supported");
            }
            const char* buf = reinterpret_cast<const char*>(v_buffer.data());
            of.write(buf, v_buffer.size());
        }

        /// NOTE: uniqueness is not guatanteed, the return type is size_t as std::hash
        template <typename ValueType> std::size_t hashVector(const std::vector<ValueType> values)
        {
            std::size_t ret = 0UL;
            for (size_t i = 0; i < values.size(); i++)
            {
                ret ^= (std::hash<ValueType>{}(values[i]) << i + 1);
                // or use boost::hash_combine (see Discussion)
            }
            return ret;
        }

        std::string timeStamp()
        {
            // C++ 20 has <date.h>
            std::chrono::time_point t = std::chrono::system_clock::now();
            auto as_time_t = std::chrono::system_clock::to_time_t(t);
            struct tm tm;
            const char* format = "%F-%H%M%S"; // suitable as key string and file name
            char some_buffer[32];             // sufficient if there no timezone info
#ifdef _MSC_VER
            if (!::gmtime_s(&tm, &as_time_t)) // reversed parameter input sequence in MSVC
                if (std::strftime(some_buffer, sizeof(some_buffer), format, &tm))
                    return std::string{some_buffer};
#else
            if (::gmtime_r(&as_time_t, &tm)) // C++, reentry version
                if (std::strftime(some_buffer, sizeof(some_buffer), format, &tm))
                    return std::string{some_buffer};
#endif
            throw std::runtime_error("Failed to get current date and time as string");
        }

        std::string timeStampFileName(const std::string filename)
        {
            auto pos = filename.rfind(".");
            auto stem = filename.substr(0, pos);
            auto suffix = filename.substr(pos, filename.size() - pos);
            return stem + "_" + timeStamp() + suffix;
        }

    } // namespace Utilities

} // namespace PPP