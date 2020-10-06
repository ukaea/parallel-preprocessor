# A prototype of general property container based on `std::any`

**A true heterogeneous container in modern C++ based on `std::any` to mimic dynamic property**

## The design of this property container 
### type erasure by `std::any`
This property container relies on c++ RTTI. RTTI runtime cost is low for `std::any` since typeid is fast, but dynamic_cast<> is slow!

See discussion on the cost of RTTI: <https://stackoverflow.com/questions/579887/how-expensive-is-rtti>
> In GCC's preferred ABI, a class vtable always holds a pointer to a per-type RTTI structure, though it might not be used. So a typeid() call itself should only cost as much as any other vtable lookup (the same as calling a virtual member function), and RTTI support shouldn't use any extra space for each object.

For OS without C++17 compiler, `boost::any` might be used instead. It is possible to mix `boost::any` and `std::any`, tested on ubnutu 18.04, libboost 1.65.

### thread safety (currently not safe)

This property container is not thread-safe, because STL container `std::map` is used. By replacing the internal has container with Intel Thread Build Blocks(TBB)'s concurrent hash container type,  thread-safe property container might be achieved. It is not yet testec.

### json serialization

json for basic types (as already supproted by nlohmann json library) are supported out of box with default json serialization,  more user types might be supported by ADL. User serializer and deserializer can be specified when `setSerializable(key, value, serializer_function)`.

Note: this feature is under design, yet fully tested.  Also consider save data into the `DataStorage` class. 

### Other example of property container design

+ Dynamic Property Maps of boost
+ PropertyContainer of FreeCAD: support transaction, serialization, etc
+ vtkInformation: <https://vtk.org/doc/nightly/html/classvtkInformation.html>
+ json.hpp: the supported data type are very limited (literal type supported by js)

If an advanced container is needed, such as transaction control, try to use a full-fledged framework like FreeCAD's `PropertyContainer`

## compile and test
### test without cmake
 first of all, download Catch2 into this folder, 

```bash
 - on posix

  g++ -std=c++17 -Wall -I./Catch2/single_include -o PropertyContainerTest PropertyContainerTest.cpp &&
  ./PropertyContainerTest --success

 - on windows
  cl -EHsc -I%CATCH_SINGLE_INCLUDE% PropertyContainerTest.cpp PropertyContainerTest.obj &&
  ./PropertyContainerTest --success
```

### test with cmake
CMakeLists.txt is ready to use, except that Catch2 needs to config/download.