#pragma once

#include "Logger.h"
#include "PreCompiled.h"

namespace PPP
{

    /// \ingroup PPP

    /** Progress report to console and/or log file
     * */
    class AppExport Progressor
    {
    protected:
        size_t myTotalSteps;
        size_t myRemainedSteps;
        size_t myReportInterval = 10;
        size_t myReportFrequency = 100;
        bool myProgessorEnabled = false;

    public:
        Progressor(size_t totalStep)
        {
            myTotalSteps = totalStep;
            myRemainedSteps = myTotalSteps;

            myReportInterval = 100;
            if (myTotalSteps > 10000)
                myReportFrequency = 1000;
            else
            {
                myReportFrequency = 100;
            }

            if (myTotalSteps < 100)
                myProgessorEnabled = false;
            else
                myProgessorEnabled = true;

            myReportInterval = size_t(myTotalSteps / myReportFrequency);
        }

        /// report progress to log stream, external graphic progress monitor will parse the log
        void remain(size_t remained_step)
        {
            if (myRemainedSteps - remained_step > myReportInterval)
            {
                myRemainedSteps = remained_step;
                if (myProgessorEnabled)
                    VLOG_F(LOGLEVEL_PROGRESS, "processed %3.1f percent\n", progress()); // PROGRESS
                /// NOTE: please keep the text format `%3.1f percent`, as external log analyzer will parse it
            }
        }

        /// report completion to log stream, external progress monitor and exit
        void finish()
        {
            /// NOTE: please keep the text format `100.0 percent, completed`, as external log analyzer will parse it
            VLOG_F(LOGLEVEL_PROGRESS, "processed 100.0 percent, completed \n");
        }

        /// calculate progress in percentage value
        inline double progress() const
        {
            return 100.0 - (100.0 * static_cast<double>(myRemainedSteps)) / static_cast<double>(myTotalSteps);
        }
    };
} // namespace PPP
