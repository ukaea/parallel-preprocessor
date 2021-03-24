
#if PPP_USE_GTEST
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#else
#include "catch2/catch.hpp"
#endif

// usually include the user cpp file directly, not the header
#include "PPP/Parameter.h"
#include "PPP/Utilities.cpp"

using namespace PPP;

/*  example to use with TEST_F ( F means fixture)
class UtilitiesTest: public ::testing::Test{
protected:
    virtual void SetUp() {};
    PPP::ProgressIndicator *p = new PPP::ProgressIndicator(100);
};
*/

#if PPP_USE_GTEST
TEST(UtilitiesTest, HasFileExtension)
{
    auto file_name = std::string("./tmp/hello.txt");
    ASSERT_TRUE(Utilities::hasFileExt(file_name, "txt"));
    ASSERT_TRUE(Utilities::hasFileExt(file_name, "TXT"));
    ASSERT_TRUE(Utilities::hasFileExt(file_name, ".txt"));
    ASSERT_FALSE(Utilities::hasFileExt(file_name, "hello"));
}
#else
/// catch2 test
TEST_CASE("UtilitiesTest", "HasFileExtension")
{
    auto file_name = std::string("./tmp/hello.txt");
    REQUIRE(Utilities::hasFileExt(file_name, "txt"));
    REQUIRE(Utilities::hasFileExt(file_name, "TXT"));
    REQUIRE(Utilities::hasFileExt(file_name, ".txt"));
    REQUIRE(Utilities::hasFileExt(file_name, "hello") == false);
}

TEST_CASE("TimeStampTest", "TimeStampFilename")
{
    auto file_name = std::string("./tmp/hello.txt");
    REQUIRE(Utilities::timeStampFileName(file_name).size() > 0);
}

#include "PPP/UniqueId.h"
TEST_CASE("UniqueIdTest", "UniqueId")
{
    // half can represent only max number about 65000
    std::vector<double> data1 = {221426.1918721609245, -42.48329410741441, 5.5995980676636385, 142.9802437026024};
    std::vector<double> data2 = {221426.6772339942936, -42.485352435515374, 5.597924411184491, 142.98044118406958};
    std::cout << half_float::half_cast<half_float::half, double>(data1[0]) << " should be inf (out of range)"
              << std::endl;
    half_float::half hf(static_cast<float>(data1[1]));
    std::cout << static_cast<float>(hf) << " ->" << std::hex << *reinterpret_cast<std::uint16_t*>(&hf) << std::endl;
    half_float::half hf2(static_cast<float>(data2[1]));
    std::cout << static_cast<float>(hf2) << " ->" << std::hex << *reinterpret_cast<std::uint16_t*>(&hf2) << std::endl;
    for (auto d : data1)
    {
        std::cout << std::hex << UniqueId::double2uint16(d);
    }
    std::cout << std::endl;
    for (auto d : data2)
        std::cout << std::hex << UniqueId::double2uint16(d);
    std::cout << std::endl;
    // std::cout << std::hex << Utilities::uniqueId(data1) << std::endl;
    REQUIRE(UniqueId::uniqueId(data1) == UniqueId::uniqueId(data2));
}

TEST_CASE("EnumJsonTest", "EnumFromJson")
{
    json enum_str = "CPU";
    auto dp = enum_str.get<PPP::DevicePreference>();
    REQUIRE(dp == PPP::DevicePreference::CPU);

    using namespace PPP;
    json enum_i = "Linear";
    IndexPattern ip = enum_i.get<IndexPattern>();
    REQUIRE(ip == IndexPattern::Linear);
}


TEST_CASE("ParameterTest", "parameterFromJson")
{
    double myValue = 1.1;
    json j{
        {"type", std::string("double")}, {"value", myValue},
        //{"range", {0, 10.0}},
    };
    auto p = Parameter<double>::fromJson(j);
    REQUIRE(p.type == std::string("double"));
    REQUIRE(p.value == myValue);
    json jp = p.toJson();

    json value_range = json::array({0, 10});
    j["range"] = value_range;
    auto p_i = Parameter<int>::fromJson(j);
    REQUIRE(p_i.value == int(myValue)); // test mixed int and double data type

    // enum type testing
    PPP::DevicePreference dev = PPP::DevicePreference::CPU;
    json je{
        {"type", "enum"}, // integer
        {"value", dev},
        {"range", {PPP::DevicePreference::CPU, PPP::DevicePreference::GPU}},
    };
    auto d = Parameter<PPP::DevicePreference>::fromJson(je);
    REQUIRE(d.value == PPP::DevicePreference::CPU);

    // this type has converter to and from json
    json je_str{
        {"type", "enum"},
        {"value", "CPU"},
        {"range", {"CPU", "GPU"}},
    };
    auto d_str = Parameter<PPP::DevicePreference>::fromJson(je_str);
    REQUIRE(d_str.value == PPP::DevicePreference::CPU);
}



#endif