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

    } // namespace Utilities

    typedef std::uint64_t UniqueIdType;
    /* *
     * these functions are used by "GeometryPropertyBuilder" to calc shape unique Id
     * from 4 key values: volume (for solid) or surface area, plus centre of mass cooordiante (x, y, z)
     * all geometry (shapes) are saved as a compound of solids, for each solid readback,
     * using methods in "GeometryPropertyBuilder" to calc 4 key values. then calc unique Id
     * using this unique Id, to match and locate meta information (json file) for each solid
     *
     * dependencies: half.hpp, json.hpp
     * */
    namespace UniqueId
    {

        typedef std::uint16_t IdItemType;

        /**
         * because half precision's max number is about 65000 (with 32 as EPS)
         * CAD ususally use mm as length unit, so volume would overflow for half precision if volume is
         * scale to deci-meter, or metre, kilometer, depends on geometry size
         * */

        // if the value is close to zero after scaling, regarded as zero in comparison
        const static double ZERO_THRESHOLD = 0.1; // changed from 1e-4 to 1 on Oct 2020, issue 20
        /// rounding: mask out east significant bits for approximately comparison
        const static IdItemType ROUND_PRECISION_MASK = 0x0008;
        const int ID_ITEM_COUNT = 4; // 4 float point numbers will be converted
        const int ID_ITEM_BITS = 16; // each float number is converted into 16bit unsigned int

        /// for unsigned int only
        template <class T> static T round(T value, T tol)
        {
            T remained = value % (tol);
            T r = value / (tol);
            if (remained >= (tol / 2))
                r += 1;
            return r * (tol);
        }

        /// map from double float to half precision float (binary16)
        /// with EPS approximately 0.001 (when the value is near 1), the max value 65000.
        /// by applying a mask (drops some sigificant bits) before hand, the EPS is 0.01  (when the value is near 1)
        /// bit_mask can be used to further reduced EPS
        /// IEEE754 the least significant bit at the lower bit when in register
        /// then from half float to underneath uint16_t by `reinterpret_cast`
        /// this function use third-party lib: HalfFloat,
        /// Note: if the value is close to zero (LENGTH_ZERO_THRESHOLD), then set it as zero,
        IdItemType double2uint16(double value)
        {
            IdItemType round_precision = ROUND_PRECISION_MASK;
            if (std::fabs(value) < ZERO_THRESHOLD)
                value = 0.0;
#if USE_HALF_FLOAT
            using namespace half_float;
            half hf(static_cast<float>(value));
            IdItemType* p = reinterpret_cast<IdItemType*>(&hf);
            IdItemType ret = round(*p, round_precision);
            return ret;
#endif
        }

        /// due to unlikely floating point error, calculated Id can be different after rounding
        std::set<UniqueIdType> nearbyIds(UniqueIdType id)
        {
            std::set<UniqueIdType> ids;
            std::vector<std::array<UniqueIdType, 3>> items;
            for (int i = 0; i < ID_ITEM_COUNT; i++)
            {
                UniqueIdType s = ROUND_PRECISION_MASK * (1UL << ID_ITEM_BITS * i);
                items.push_back({id - s, id, id + s}); // overflow and underflow is fine
            }
            // generate combination,  3**4, generate ijkl index
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    for (int k = 0; k < 3; k++)
                    {
                        for (int l = 0; l < 3; l++)
                        {
                            UniqueIdType shift = items[0][i] + items[0][j] + items[0][k] + items[0][l];
                            ids.insert(id + shift);
                        }
                    }
                }
            }
            return ids;
        }

        /// consider
        bool uniqueIdEqual(UniqueIdType id1, UniqueIdType id2)
        {
            auto nearby_ids = nearbyIds(id2);
            return nearby_ids.find(id1) != nearby_ids.end();
            /*
                for (size_t i = 0; i < ID_ITEM_COUNT; i++)
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
                */
        }

        /// used by GeometryPropertyBuilder class in parallel-preprocessor
        /// assuming native endianess, always calculate Id using the same endianness
        /// this should generate unique Id for a vector of 4 double values
        /// this is not universal unique Id (UUID) yet, usurally std::byte[16]
        UniqueIdType uniqueId(const std::vector<double> values)
        {
            UniqueIdType ret = 0x00000000;
            assert(values.size() == ID_ITEM_COUNT);

            for (size_t i = 0; i < values.size(); i++)
            {
                UniqueIdType tmp = double2uint16(values[i]);
                ret ^= (tmp << i * ID_ITEM_BITS); // XOR is fine, but  OR is more readable
            }
            return ret;
        }

        std::vector<double> uniqueIdToGeometry(UniqueIdType id)
        {
            std::vector<double> values;
            for (int i = 0; i < ID_ITEM_COUNT; i++)
            {
#if USE_HALF_FLOAT
                using namespace half_float;
                IdItemType hf = (id >> (i * ID_ITEM_BITS)) & 0xffff;
                half* p = reinterpret_cast<half*>(&hf);
                double v = static_cast<double>(*p);
                values.push_back(v);
#endif
            }
            return values;
        }

        /// from float array to int[] then into a hash value
        UniqueIdType geometryUniqueId(double volume, std::vector<double> centerOfMass, const double LENGTH_SCALE = 0.1)
        {
            double gp = std::pow(volume, 1.0 / 3.0);
            std::vector<double> pv{gp * LENGTH_SCALE, centerOfMass[0] * LENGTH_SCALE, centerOfMass[1] * LENGTH_SCALE,
                                   centerOfMass[2] * LENGTH_SCALE};
            return uniqueId(pv);
        }
    } // namespace UniqueId
} // namespace PPP