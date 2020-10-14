/***************************************************************************
 *   Copyright (c) Riegel         <juergen.riegel@web.de>                  *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_BASECLASS_H
#define BASE_BASECLASS_H

#include "./Type.h"

#if PPP_BUILD_PYTHON
// Python stuff, temperary make it opaque
#include <Python.h>
// can be done by pybind11:Object
//#include "PyObjectBase.h"  // class BaseExport PyObjectBase : public PyObject
#endif

/** \addtogroup Base
 * @{
 * */

/// define for subclassing Base::BaseClass, override has been added for C++11
#define TYPESYSTEM_HEADER()                                                                                            \
public:                                                                                                                \
    static Base::Type getClassTypeId(void);                                                                            \
    virtual Base::Type getTypeId(void) const override;                                                                 \
    static void init(void);                                                                                            \
    static void destructType(void);                                                                                    \
    static void* create(void);                                                                                         \
                                                                                                                       \
private:                                                                                                               \
    static Base::Type classTypeId


/// deprecated: Like TYPESYSTEM_HEADER, but declare getTypeId as 'override'
#define TYPESYSTEM_HEADER_WITH_OVERRIDE()                                                                              \
public:                                                                                                                \
    static Base::Type getClassTypeId(void);                                                                            \
    virtual Base::Type getTypeId(void) const override;                                                                 \
    static void init(void);                                                                                            \
    static void destructType(void);                                                                                    \
    static void* create(void);                                                                                         \
                                                                                                                       \
private:                                                                                                               \
    static Base::Type classTypeId


/// define to implement a  subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_P(_class_)                                                                                   \
    Base::Type _class_::getClassTypeId(void)                                                                           \
    {                                                                                                                  \
        return _class_::classTypeId;                                                                                   \
    }                                                                                                                  \
    Base::Type _class_::getTypeId(void) const                                                                          \
    {                                                                                                                  \
        return _class_::classTypeId;                                                                                   \
    }                                                                                                                  \
    Base::Type _class_::classTypeId = Base::Type::badType();                                                           \
    void* _class_::create(void)                                                                                        \
    {                                                                                                                  \
        return new _class_();                                                                                          \
    }                                                                                                                  \
    void _class_::destructType(void)                                                                                   \
    {                                                                                                                  \
        _class_::classTypeId = Base::Type::badType();                                                                  \
    }

/// define to implement a subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_ABSTRACT_P(_class_)                                                                          \
    Base::Type _class_::getClassTypeId(void)                                                                           \
    {                                                                                                                  \
        return _class_::classTypeId;                                                                                   \
    }                                                                                                                  \
    Base::Type _class_::getTypeId(void) const                                                                          \
    {                                                                                                                  \
        return _class_::classTypeId;                                                                                   \
    }                                                                                                                  \
    Base::Type _class_::classTypeId = Base::Type::badType();                                                           \
    void* _class_::create(void)                                                                                        \
    {                                                                                                                  \
        return 0;                                                                                                      \
    }                                                                                                                  \
    void _class_::destructType(void)                                                                                   \
    {                                                                                                                  \
        _class_::classTypeId = Base::Type::badType();                                                                  \
    }

/// define to implement a subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE(_class_, _parentclass_)                                                                      \
    TYPESYSTEM_SOURCE_P(_class_);                                                                                      \
    void _class_::init(void)                                                                                           \
    {                                                                                                                  \
        initSubclass(_class_::classTypeId, #_class_, #_parentclass_, &(_class_::create), &(_class_::destructType));    \
    }


/// define to implement a subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_ABSTRACT(_class_, _parentclass_)                                                             \
    TYPESYSTEM_SOURCE_ABSTRACT_P(_class_);                                                                             \
    void _class_::init(void)                                                                                           \
    {                                                                                                                  \
        initSubclass(_class_::classTypeId, #_class_, #_parentclass_, &(_class_::create), &(_class_::destructType));    \
    }

/// define to implement a  subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_TEMPLATE(_class_, _parentclass_)                                                             \
    template <> Base::Type _class_::getClassTypeId(void)                                                               \
    {                                                                                                                  \
        return _class_::classTypeId;                                                                                   \
    }                                                                                                                  \
    template <> Base::Type _class_::getTypeId(void) const                                                              \
    {                                                                                                                  \
        return _class_::classTypeId;                                                                                   \
    }                                                                                                                  \
    template <> Base::Type _class_::classTypeId = Base::Type::badType();                                               \
    template <> void* _class_::create(void)                                                                            \
    {                                                                                                                  \
        return new _class_();                                                                                          \
    }                                                                                                                  \
    template <> void _class_::destructType(void)                                                                       \
    {                                                                                                                  \
        _class_::classTypeId = Base::Type::badType();                                                                  \
    }                                                                                                                  \
    template <> void _class_::init(void)                                                                               \
    {                                                                                                                  \
        initSubclass(_class_::classTypeId, #_class_, #_parentclass_, &(_class_::create), &(_class_::destructType));    \
    }

/** @} */

namespace Base
{
    /// \ingroup Base
    /// BaseClass class and root of the type system
    class BaseExport BaseClass
    {
    public:
        static Type getClassTypeId(void);
        virtual Type getTypeId(void) const;
        std::string className(void) const
        {
            return getTypeId().getName();
        }
        bool isDerivedFrom(const Type type) const
        {
            return getTypeId().isDerivedFrom(type);
        }
        static void init(void);
        static void destructType(void);

#if PPP_BUILD_PYTHON
        virtual PyObject* getPyObject(void);
        virtual void setPyObject(PyObject*);
#endif
        static void* create(void)
        {
            return nullptr;
        }

    private:
        static Type classTypeId;

    protected:
        static void initSubclass(Base::Type& toInit, const char* ClassName, const char* ParentName,
                                 Type::instantiationMethod method = 0, Type::destructTypeMethod destruct = 0);

    public:
        /// Construction
        BaseClass();
        /// Destruction
        virtual ~BaseClass();
    };

    /**
     * Template that works just like dynamic_cast,
     * but expects the argument to inherit from Base::BaseClass.
     */
    template <typename T> T* type_dynamic_cast(Base::BaseClass* t)
    {
        if (t && t->isDerivedFrom(T::getClassTypeId()))
            return static_cast<T*>(t);
        else
            return nullptr;
    }

    /**
     * Template that works just like dynamic_cast,
     * but expects the argument to inherit from a const Base::BaseClass.
     */
    template <typename T> const T* type_dynamic_cast(const Base::BaseClass* t)
    {
        if (t && t->isDerivedFrom(T::getClassTypeId()))
            return static_cast<const T*>(t);
        else
            return nullptr;
    }


} // namespace Base

#endif // BASE_BASECLASS_H
