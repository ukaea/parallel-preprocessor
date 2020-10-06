#pragma once

#include "./TypeDefs.h"

namespace PPP
{

    typedef json Response;
    enum AppExport MessageType
    {
        Request, ///> the operator must give response within timeout
        Log      ///> just record the operations
    };

    /** request sent by OperatorProxy to be processed by operator
     * Work-in-progress
     */
    class AppExport Transaction
    {
    public:
        // C++11 can do in-place Default Initializers for Member Variables (value types only)
        MessageType messageType = MessageType::Request;

        float timeout = 10.0f; /// seconds
        json payload{};
        json request{}; /// optional, only for Request Message Type

        Transaction() = default;
    };


    /** abstract interface for user (operator) interaction
     * */
    class AppExport AbstractOperator
    {
    public:
        virtual bool secure() = 0;
        virtual void display(const json&) = 0;
        virtual void report(const json&) = 0;

        // virtual void progress(const float percentage) = 0;  // report progress
        // virtual const json request(const json&) = 0;
    };

} // namespace PPP