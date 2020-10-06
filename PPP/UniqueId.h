#pragma once

#include <cassert>
#include <cstdint>
#include <vector>

#define USE_HALF_FLOAT 1
#if USE_HALF_FLOAT
#include "third-party/half.hpp"
#else
#error "in-house double to unit16_t mapping is not yet implemented"
#endif



namespace PPP
{

    namespace Utilities
    {

        /* *
         * these functions are used by "GeometryPropertyBuilder" to calc shape unique Id
         * from 4 key values: volume (for solid) or surface area, plus centre of mass cooordiante (x, y, z)
         * all geometry (shapes) are saved as a compound of solids, for each solid readback,
         * using methods in "GeometryPropertyBuilder" to calc 4 key values. then calc unique Id
         * using this unique Id, to match and locate meta information (json file) for each solid
         *
         * dependencies: half.hpp, json.hpp
         * */
        typedef uint64_t UniqueIdType;

        /// detect byte order, alternatively, GCC has `__BYTE_ORDER__` macro
        /// little endianness is more popular in CPU worlds
#ifdef __clang__
        // constexpr for this function is not supported by CLANG version 6, 10
        // `error: constexpr function never produces a constant expression`
        bool isBigEndian(void)
#else
        constexpr bool isBigEndian(void)
#endif
        {
            union
            {
                uint32_t i;
                char c[4];
            } bint = {0x01020304};

            return bint.c[0] == 1;
        }

        /// for unsigned int only
        template <class T> static T round(T value, T tol)
        {
            T remained = value % (tol);
            T r = value / (tol);
            if (remained >= (tol / 2))
                r += 1;
            return r * (tol);
        }

        /**
         * because half precision max number is 65000
         * CAD ususally use mm as length unit, so volume would overflow for half precision
         * scale to deci-meter, or metre, kilometer, depends on geometry size
         * */
        const double LENGTH_SCALE = 0.01;
        // if the value is close to zero after scaling, regarded as zero in comparison
        const static double ZERO_THRESHOLD = 1e-4;
        /// rounding: mask out east significant bits for approximately comparison
        const std::uint16_t ROUND_PRECISION_MASK = 0x0008;


        /// map from double float to half precision float (binary16)
        /// with EPS approximately 0.001 (when the value is near 1), the max value 65000
        /// bit_mask can be used to further reduced EPS
        /// IEEE754 the least signicant bit at the lower bit when in register
        /// then from half float to underneath uint16_t by `reinterpret_cast`
        /// this function use third-party lib: HalfFloat,
        /// Note: if the value is close to zero (LENGTH_ZERO_THRESHOLD), then set it as zero,
        std::uint16_t double2uint16(double value)
        {
            std::uint16_t round_precision = ROUND_PRECISION_MASK;
            if (std::fabs(value) < ZERO_THRESHOLD)
                value = 0.0;
#if USE_HALF_FLOAT
            using namespace half_float;
            half hf(static_cast<float>(value));
            std::uint16_t* p = reinterpret_cast<std::uint16_t*>(&hf);
            std::uint16_t ret = round(*p, round_precision);
            return ret;
#endif
        }


        /// due to unlikely floating point error, calculated Id can be different after rounding
        std::vector<UniqueIdType> nearbyIds(UniqueIdType)
        {
            std::vector<UniqueIdType> nids;
            throw "todo:  not completed";
            return nids;
        }

        /// approximate equal by float point data comparison
        /// endianess neutral
        bool uniqueIdEqual(uint64_t id1, std::uint64_t id2)
        {
            for (size_t i = 0; i < 4; i++)
            {
                uint16_t i1 = (id1 << i * 16) & 0xFFFF;
                uint16_t i2 = (id2 << i * 16) & 0xFFFF;
                // todo: this does not consider overflow
                // NaN half precision should works
                if (i1 > i2 + ROUND_PRECISION_MASK || i1 < i2 - ROUND_PRECISION_MASK)
                {
                    return false;
                }
            }
            return true;
        }

        /// used by GeometryPropertyBuilder class in parallel-preprocessor
        /// assuming native endianess, always calculate Id using the same endianness
        /// this should generate unique Id for a vector of 4 double values
        /// this is not universal unique Id (UUID) yet, usurally std::byte[16]
        UniqueIdType uniqueId(const std::vector<double> values)
        {
            std::uint64_t ret = 0x00000000;
            assert(values.size() == 4);

            for (size_t i = 0; i < values.size(); i++)
            {
                std::uint64_t tmp = double2uint16(values[i]);
                ret ^= (tmp << i * 16);
            }
            return ret;
        }

        /// from float array to int[] then into a hash value
        UniqueIdType geometryUniqueId(double volume, std::vector<double> centerOfMass)
        {
            double gp = volume * (LENGTH_SCALE * LENGTH_SCALE * LENGTH_SCALE);
            std::vector<double> pv{gp, centerOfMass[0] * LENGTH_SCALE, centerOfMass[1] * LENGTH_SCALE,
                                   centerOfMass[2] * LENGTH_SCALE};
            return uniqueId(pv);
        }
    } // namespace Utilities
} // namespace PPP