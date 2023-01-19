
#define USE_CATCH2 1
#if USE_CATCH2
// surprisingly, CATCH_CONFIG_MAIN should be put before `#include "catch2/catch.hpp"ls`
#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#endif


#include "PropertyContainer.hpp"
using namespace PPP;
#include <any>
#include <vector>


/// a dummy class to test property container
class A
{
public:
    std::string name = "I am a class";

    A() = default;
    // A() {std::cout<< "A constructed with name" + name + "\n"; };
    A(A&& a)
            : name(std::move(a.name))
    {
        std::cout << "A constructed from move ator() with name" + name + "\n";
    };
    A(const std::string& _name)
            : name(_name)
    {
        std::cout << "A constructed by user ctor with name:" + name + "\n";
    };
    ~A()
    {
        std::cout << "A destructed with name" + name + "\n";
    };
};


TEST_CASE("test std::any user type", "[multi-file:1]")
{
    using myType = A;
    auto v = std::make_shared<myType>("A class");
    std::any a(v);
    auto data = std::any_cast<std::shared_ptr<myType>>(a);
    REQUIRE(data->name == "A class");
}

TEST_CASE("test std::any STL", "[multi-file:1]")
{
    int aNumber = 1;
    using myType = std::vector<int>;
    auto v = std::make_shared<myType>(aNumber);
    std::any a = v;
    auto data = std::any_cast<std::shared_ptr<myType>>(v);
    REQUIRE(data->size() == aNumber);
}

TEST_CASE("test Property class", "[multi-file:2]")
{
    Property p("aKey", std::any(std::make_shared<A>()));
    // std::cout << p.name << "\n";
    Property p1 = std::move(p);
    REQUIRE(p1.name == "aKey");
    REQUIRE(p.name == "");
    // Property p2(p1);  // copy constructor is private
}


// actually TEST_CASE with SECTION is enough
class PropertyContainerTest
{
protected:
    PropertyContainer d;
    // no need for `void SetUp() override`
};

/*
 addProperty() are make protected after testing successfully
TEST_CASE_METHOD(PropertyContainerTest, "add and remove properties", "[multi-file:3]")
{
    PropertyContainer d;
    d.addProperty(Property("aKey", std::any(std::make_shared<A>())));
    auto pp = d.getProperty("aKey");
    // std::cout << "getProperty:" << pp.name <<std::endl;
    REQUIRE(pp.name == "aKey");

    // d.addProperty(Property("aKey", a));
    // auto aa = std::any_cast<A>(d["aKey"]);
}
*/

TEST_CASE_METHOD(PropertyContainerTest, "add and remove a user defined class", "[multi-file:4]")
{
    PropertyContainer d;
    d.set<A>("A", std::make_shared<A>());
    d.set<A>("A", std::make_shared<A>("I am a class too"));
    auto asp = d.get<A>("A"); // second template parameter is default to `std::shared_ptr<A>`
    // std::cout<< asp->name <<"\n";
    REQUIRE(asp->name == "I am a class too");
    INFO("It is found to have such a failure, since overriding an existing key is skipped");
    d.erase("A"); // no effect if no such key
}

TEST_CASE_METHOD(PropertyContainerTest, "add and remove a STL container as a property data", "[multi-file:4]")
{
    PropertyContainer d;
    std::size_t length = 100;
    using B = std::vector<int>;
    d.set<B>("B", std::make_shared<B>(length));
    auto b = d.get<B>("B"); // second template parameter is default to `std::shared_ptr<A>`
    // std::cout<< "size of std container is: " << b->size() <<"\n";
    REQUIRE(b->size() == length);

    b->push_back(1);
    auto bb = d.get<B>("B"); // second template parameter is default to `std::shared_ptr<A>`
    // std::cout<< "size of the modified std container is: " << bb->size() <<"\n";
    // const char* MY_MSG = "modified size of the modified stl container is NOT sync with the property Container";
    REQUIRE(bb->size() == length + 1U);
    d.erase("B"); // no effect if no such key
}


TEST_CASE_METHOD(PropertyContainerTest, "add and remove a STL container as a value property data 2", "[multi-file:5]")
{
    PropertyContainer d;
    std::size_t length = 100;
    using B = std::vector<int>;
    B b = B(length);
    d.emplace<B>("B", std::move(b));
    auto bp = d.extract<B>("B"); // second template parameter is default to `std::shared_ptr<A>`
    // std::cout<< "size of std container is: " << b->size() <<"\n";
    REQUIRE(bp->size() == length);
    REQUIRE(d.contains("B") == false);
}

TEST_CASE_METHOD(PropertyContainerTest, "template deduction and overload test", "[multi-file:6]")
{
    PropertyContainer d;
    std::size_t length = 5;
    using Data = std::vector<int>;
    Data data = Data(length, 1);
    // B& ref = cdata;
    d.set("C", std::move(data)); // template deduction
    REQUIRE(d.get<Data>("C")->size() == length);

    // return type template deduction may help
    // get const data is fine, even data inside is non-const
    // REQUIRE(d.get<const Data>("C")->size() == length);
    REQUIRE(d.getConst<Data>("C")->size() == length);

    // if it is pointer, it must provide the template parameter type
    d.set<Data>("CP", std::make_shared<Data>(length));
    REQUIRE(d.get<Data>("CP")->size() == length);

    data.resize(length, 1); // refill the data
    const Data& cref = data;
    d.setValue<Data>("CP_V", cref);
    REQUIRE(d.get<Data>("CP_V")->size() == length);

    // test const, const can not be moved
    d.set<const Data>("CP_C", std::make_shared<const Data>(length));
    // d.set("CP_CC", std::make_shared<const Data>(length));  //failed
    REQUIRE(d.get<const Data>("CP_C")->size() == length);

    data.resize(length, 2);                    // refill the data
    d.setSerializable("C_S", std::move(data)); // template deduction
    REQUIRE(d.get<Data>("C_S")->size() == length);

    d.setSerializable<const Data>("CP_S", std::make_shared<const Data>(length, 2));
    REQUIRE(d.get<const Data>("CP_S")->size() == length);

    /*
    const Data cdata = {1, 2, 3, 4, 5};
    d.setSerializable("C", ); // template deduction
    */
}

TEST_CASE_METHOD(PropertyContainerTest, "json serialization test", "[multi-file:7]")
{
    PropertyContainer d;
    d.setSerializable<int>("A", std::make_shared<int>(1));

    std::size_t length = 5;
    using B = std::vector<int>;
    // B b = B(length);
    d.setSerializable<B>("B", std::make_shared<B>(length));

    B data = {1, 2, 3, 4, 5};
    d.setSerializable("D", std::move(data));


    std::string filepath = "filename_for_this_data";
    d.setSerializable<B>("E", std::make_shared<B>(2), filepath);

    //==============================
    std::string filename = "dump_test.json";
    // std::ofstream output(filename);
    d.save(filename);

    std::ifstream input(filename);
    json j;
    input >> j;

    REQUIRE(j.contains("A"));
    REQUIRE(j.contains("B"));
    REQUIRE(j["D"]["data"].size() == length);
    REQUIRE(j["E"]["data"] == filepath);
}
