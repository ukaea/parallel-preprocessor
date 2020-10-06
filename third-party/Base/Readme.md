# Cross-platform C++ BaseClass and type system

This is a source code level integration of an external repository, extracted from FreeCAD project.
see:  https://github.com/qingfengxia/cppBase  LGPL v2.1
split out this module, and then download at compiling time, if this repo will be released in a different license.

Features:
+ Base class for C++
+ Type system:  extracted from FreeCAD's Base module <>
+ Design pattern
+ template class are supported by a new macro `TYPE_SOURCE_TEMPLATE`


Todo:
- shared_ptr<> replace all void*
- python wrapping helper
- cross-platform, OS, compiler `compatible.h`
- helper methods into BaseClass



## BaseClass

### Java and C# base class
C++ does not have a base/root class for all objects, but lot of other high level languages have.

For a few C++ framework like QT, VTK, GTK, there is base class to provide shared functions
see: 
[QT: `QObject`](https://doc.qt.io/qt-5/qobject.html)
[VTK: `vtkObjectBase` and `vtkObject`](https://vtk.org/doc/nightly/html/classvtkObjectBase.html)


### typical functions of base class

1. type system, implemented by c++ macro
2. reference counting, C++11 shared_pointer<> has this function
3. event/observer/subscription pattern, depends on the design of the framework

### help on script wrapping


## Type system 
### Type system for C++

C++ does not support runtime reflection to get class name, create instance from name (possible by factory pattern).

It does not relies on dynamic type information.
It does not require C++11 compiler.

Benefits: 
+ create instance from string name
+ get printable class name
+ inheritance tree

Qt's type or meta data system is more powerful, while if you need such a powerful system, just use QT. 

### top up on FreeCAD 

1. error string, when forget to init/register the class
2. it is thread-unsafe, for type init, is that safe afterward?
3. template class support
4. example code <./TypeTest.cpp>
5. types can be registered by `class::init()` and `Type::destruct()` and `init()` again

### usage

1. header file
In each class's header file (inc. header only class), `TYPESYSTEM_HEADER();` must be the first line of that class
Just like `Q_CLASS` for Qt meta system.

```
class CClass : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    int d = 0;
};
```

2. source file
In very beginnning of the source file for that class, not within class scope. Or any other cpp file for header only class.
`TYPESYSTEM_SOURCE(CClass, Base::BaseClass);`
header only is not supported for static member data declaration. 

3. main source file
To use this type system: in `main()` or module initialization function (called in `main()`)
```
int main()
{
    using namespace Base;

    Type::init(); // init the type system
    // then init each class (actually register this class type), including the BaseClass
    BaseClass::init();  // this root class must be initialized

    // user classes init, can be wrap in a module init()
    CClass::init();



    Type::destruct();
    return 0;
}
```

see the [TypeTest source](TypeTest.cpp)
