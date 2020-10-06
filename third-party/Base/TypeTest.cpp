#include "BaseClass.h"
#include <iostream>



class CClass : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    int d = 0;
};
TYPESYSTEM_SOURCE(CClass, Base::BaseClass);

// clang does not support this, but g++ is fine
#ifdef __GNUC__
#ifndef __clang__
template <class T> class TClass : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    T data;
};
TYPESYSTEM_SOURCE_TEMPLATE(TClass<int>, Base::BaseClass);
#endif
#endif

int main()
{
    using namespace Base;

    Type::init(); // init the type system
    // then init each class (actually register this class type), including the BaseClass
    BaseClass::init(); // this root class must be initialized
    // user classes init
    CClass::init();

#ifdef __GNUC__
#ifndef __clang__
    TClass<int>::init();
    BaseClass* t = new TClass<int>();
    Type tt = t->getTypeId();
    std::cout << tt.getName() << std::endl;
#endif
#endif

    BaseClass* b = new BaseClass();
    Type bt = b->getTypeId();
    std::cout << bt.getName() << std::endl;

    Base::BaseClass* bc = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName("CClass"));
    CClass* c2 = type_dynamic_cast<CClass>(bc);

    const char* sType = "CClass";
    Type ctt = Type::fromName(sType);

    CClass* cp;
    auto tmp = Base::Type::createInstanceByName("CClass");
    Base::BaseClass* base = static_cast<Base::BaseClass*>(tmp);
    if (base)
    {
        if (!base->getTypeId().isDerivedFrom(Base::BaseClass::getClassTypeId()))
        {
            delete base;
            std::cout << "'" << sType << "' is not a Base::BaseClass type";
        }
        cp = static_cast<CClass*>(base);
    }
    // auto a = Type::createInstanceByName("AClass"); // return void*
    // AClass* Ap = type_dynamic_cast<AClass>(a);
    std::cout << cp->getClassTypeId().getName() << std::endl;

    // delete, or shared_ptr<>

    Type::destruct(); // also call destructType() of all types
    //============================================================
    Type::init(); // init the type system
    // then init each class (actually register this class type), including the BaseClass
    BaseClass::init(); // this root class must be initialized
    // user classes init
    CClass::init();

    CClass* c = new CClass();
    Type ct = c->getTypeId();
    std::cout << ct.getName() << std::endl;

    return 0;
}