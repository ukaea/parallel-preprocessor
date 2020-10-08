#pragma once

#include <exception>
#include <string>

namespace PPP
{
    /**
     * base class for all processor error, derived from std::exception
     */
    class AppExport ProcessorError : public std::exception
    {
    protected:
        /** Error message
         */
        std::string myMessage;

    public:
        // using std::exception::exception;
        explicit ProcessorError(const char* message)
                : myMessage(message)
        {
        }
        explicit ProcessorError(const std::string& message)
                : myMessage(message)
        {
        }

        /** Destructor.
         * Virtual to allow for subclassing.
         */
        virtual ~ProcessorError() throw()
        {
        }

        /** Returns a pointer to the (constant) error description.
         *  @return A pointer to a const char*. The underlying memory
         *          is in posession of the Exception object. Callers must
         *          not attempt to free the memory.
         */
        virtual const char* what() const throw() override
        {
            return myMessage.c_str();
        }
    };
} // namespace PPP