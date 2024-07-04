extern "C"
{
#include "JsonParser.h"
}

#define BOOST_TEST_MODULE jsonParser
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <string>
#include <iostream>
#include <vector>

struct JpathToExpectation
{
    std::string jpath;
    std::string expectation;
};

bool operator== (const JpathToExpectation& lhs, const JpathToExpectation& rhs)
{
    return lhs.jpath == rhs.jpath && lhs.expectation == rhs.expectation;
}

std::ostream& operator << (std::ostream& os, const JpathToExpectation& lhs)
{
    return os << lhs.jpath << ": " << lhs.expectation;
}

void print(const char* key, int keyLen, const char* value, int valueLen)
{
    std::string keyStr(key, keyLen);
    std::string valuestr(value, valueLen);

    std::cout << keyStr << ": " << valuestr << std::endl;
}

std::vector<JpathToExpectation> expectations;

struct JsonToExpectation
{
    std::string json;
    std::vector<JpathToExpectation> expectations;
};

std::ostream& operator << (std::ostream& os, const JsonToExpectation& lhs)
{
    return os << lhs.json;
}

void doNothing(const char* key, int keyLen, const char* value, int valueLen)
{
}

void check(const char* key, int keyLen, const char* value, int valueLen)
{
    JpathToExpectation ex{ std::string (key, keyLen), std::string(value, valueLen)};
    auto it = std::find(expectations.begin(), expectations.end(), ex);
    bool found = it != std::end(expectations);
    if (not found)
    {
        std::cout << ex << std::endl;

        std::cout << "Expectations left:" << std::endl;
        for (auto&& elem : expectations)
        {
            std::cout << elem << std::endl;
        }
    }
    BOOST_TEST(found);
    if (found)
    {
        expectations.erase(it);
    }
}

BOOST_AUTO_TEST_CASE(shall_parse_emptyStr)
{
    expectations = {
        {"", "{}"},
    };

    std::string s = R"^^^({})^^^";
    JsonParser json;

    BOOST_TEST(0 == JsonParser_parse(&json, s.c_str(), s.c_str() + s.size(), check));
    BOOST_TEST(0 == expectations.size());
}

BOOST_AUTO_TEST_CASE(shall_parse_text_only_json)
{
    expectations = {
        {"", "\"Hello world!\""},
    };

    std::string s = R"^^^("Hello world!")^^^";
    JsonParser json;

    BOOST_TEST(0 == JsonParser_parse(&json, s.c_str(), s.c_str() + s.size(), check));
    BOOST_TEST(0 == expectations.size());
}

std::vector<JsonToExpectation> stringWithTerminatedCharacters{
    {R"^^^("Text With terminated \" quote")^^^", {{"", "\"Text With terminated \\\" quote\""}}},
    {R"^^^({ "keyWith\/EscapedChar": "value\taa" })^^^", {
        {"/keyWith\\/\EscapedChar",  "\"value\\taa\""},
        {"", R"^^^({ "keyWith\/EscapedChar": "value\taa" })^^^"}
    }}
};
BOOST_DATA_TEST_CASE(shallParseTerminatedStrings, stringWithTerminatedCharacters, arg)
{
    expectations = arg.expectations;

    JsonParser parser;

    BOOST_TEST(0 == JsonParser_parse(&parser, arg.json.c_str(), arg.json.c_str() + arg.json.size(), check));
    BOOST_TEST(0 == expectations.size());
}

BOOST_AUTO_TEST_CASE(support_spaces_in_keys)
{
    expectations = {
        {"/Image nr 1/Width a", "800"},
        {"/Image nr 1/Width b", "900"},
        {"/Image nr 1", R"^^^({ "Width a": 800, "Width b": 900 })^^^" },
        {"", R"^^^({ "Image nr 1": { "Width a": 800, "Width b": 900 } })^^^" }
    };

    std::string s = R"^^^({ "Image nr 1": { "Width a": 800, "Width b": 900 } })^^^";
    JsonParser json;

    BOOST_TEST(0 == JsonParser_parse(&json, s.c_str(), s.c_str() + s.size(), check));
    BOOST_TEST(0 == expectations.size());
}


std::vector<JsonToExpectation> numbersOnly
{
    { R"^^^(0)^^^", {{"", "0"}} },
    { R"^^^(-1)^^^", {{"", "-1"}} },
    { R"^^^(42)^^^", {{"", "42"}} },
    { R"^^^(1234567890)^^^", {{"", "1234567890"}} },
    { R"^^^(37.1)^^^", {{"", "37.1"}} },
    { R"^^^(1234567890.1234567890)^^^", {{"", "1234567890.1234567890"}} },
    { R"^^^(1e1)^^^", {{"", "1e1"}} },
    { R"^^^(1E1)^^^", {{"", "1E1"}} },
    { R"^^^(1E+1)^^^", {{"", "1E+1"}} },
    { R"^^^(1E-1)^^^", {{"", "1E-1"}} },
};
BOOST_DATA_TEST_CASE(shall_parse_number_only_json, numbersOnly, arg)
{
    expectations = arg.expectations;

    JsonParser parser;

    BOOST_TEST(0 == JsonParser_parse(&parser, arg.json.c_str(), arg.json.c_str() + arg.json.size(), check));
    BOOST_TEST(0 == expectations.size());
}

std::vector<JsonToExpectation> notCorrectNumbers
{
    { R"^^^(-)^^^", {} },
    { R"^^^(-0)^^^", {} },
    { R"^^^(1.)^^^", {} },
    { R"^^^(1.e)^^^", {} },
    { R"^^^(1e)^^^", {} },
    { R"^^^(1e+)^^^", {} },
    { R"^^^(1e-)^^^", {} },
    { R"^^^(1E)^^^", {} },
    { R"^^^(1E+)^^^", {} },
    { R"^^^(1E-)^^^", {} },
};
BOOST_DATA_TEST_CASE(shallNotParseIncorrectNumbers, notCorrectNumbers, arg)
{
    JsonParser parser;

    BOOST_TEST(0 != JsonParser_parse(&parser, arg.json.c_str(), arg.json.c_str() + arg.json.size(), check));
}

std::vector<JsonToExpectation> basicLiterals{
    {R"^^^(true)^^^", {{"", "true"}}},
    {R"^^^(null)^^^", {{"", "null"}}},
    {R"^^^(false)^^^", {{"", "false"}}},
};
BOOST_DATA_TEST_CASE(shallParseBasicLiterals, basicLiterals, arg)
{
    expectations = arg.expectations;

    JsonParser parser;

    BOOST_TEST(0 == JsonParser_parse(&parser, arg.json.c_str(), arg.json.c_str() + arg.json.size(), check));
    BOOST_TEST(0 == expectations.size());
}

std::vector<JsonToExpectation> notCorrectLiterals{
    {R"^^^(True)^^^", {}},
    {R"^^^(Null)^^^", {}},
    {R"^^^(False)^^^", {}},
    {R"^^^(xtrue)^^^", {}},
    {R"^^^(xnull)^^^", {}},
    {R"^^^(xfalse)^^^", {}},
    {R"^^^(truex)^^^", {}},
    {R"^^^(nullx)^^^", {}},
    {R"^^^(falsex)^^^", {}},
    {R"^^^(false, false)^^^", {}},
    {R"^^^(false,false)^^^", {}},
};
BOOST_DATA_TEST_CASE(shallNotParseIncorrectLiterals, notCorrectLiterals, arg)
{
    JsonParser parser;

    BOOST_TEST(0 != JsonParser_parse(&parser, arg.json.c_str(), arg.json.c_str() + arg.json.size(), doNothing));
}

std::vector<JsonToExpectation> arrays{
    {R"^^^([])^^^", {{"", "[]"}}},
    {R"^^^([null])^^^", {{"[0]", "null"}, {"", "[null]"}}},
    {R"^^^([null, null])^^^", {{"[0]", "null"}, {"[1]", "null"}, {"", "[null, null]"}}},
    {R"^^^([1, "null"])^^^", {{"[0]", "1"}, {"[1]", "\"null\""}, {"", "[1, \"null\"]"}}},
};
BOOST_DATA_TEST_CASE(shallParseArrays, arrays, arg)
{
    expectations = arg.expectations;

    JsonParser parser;

    BOOST_TEST(0 == JsonParser_parse(&parser, arg.json.c_str(), arg.json.c_str() + arg.json.size(), check));
    BOOST_TEST(0 == expectations.size());
}

/* Failed scenarios. */
std::vector<JsonToExpectation> incorrectsJsons{
    {R"^^^({)^^^", {}},
    {R"^^^(})^^^", {}},
    {R"^^^([)^^^", {}},
    {R"^^^(])^^^", {}},
    {R"^^^({,})^^^", {}},
    {R"^^^([,])^^^", {}},
};
BOOST_DATA_TEST_CASE(shall_return_error_in_incorrect_json, incorrectsJsons, arg)
{
    std::string s = R"^^^({)^^^";
    JsonParser parser;
    BOOST_TEST(0 != JsonParser_parse(&parser, s.c_str(), s.c_str() + s.size(), doNothing));
}

BOOST_AUTO_TEST_CASE(shall_parse_example)
{
    expectations = {
        {"/Image/Width", "800"},
        {"/Image/Height", "600"},
        {"/Image/Title", "\"View from 15th Floor\""},
        {"/Image/Thumbnail/Url", "\"http://www.example.com/image/481989943\""},
        {"/Image/Thumbnail/Height", "125"},
        {"/Image/Thumbnail/Width", "100"},
        {"/Image/Thumbnail", R"^^^({ "Url": "http://www.example.com/image/481989943", "Height" : 125, "Width" : 100 })^^^"},
        {"/Image/Animated", "false"},
        {"/Image/IDs[0]", "116"},
        {"/Image/IDs[1]", "943"},
        {"/Image/IDs[2]", "234"},
        {"/Image/IDs[3]", "38793"},
        {"/Image/IDs", "[116, 943, 234, 38793]"},
        {"/Image", R"^^^({ "Width": 800, "Height" : 600, "Title" : "View from 15th Floor", "Thumbnail" : { "Url": "http://www.example.com/image/481989943", "Height" : 125, "Width" : 100 }, "Animated" : false, "IDs" : [116, 943, 234, 38793] })^^^"},
        {"", R"^^^({ "Image": { "Width": 800, "Height" : 600, "Title" : "View from 15th Floor", "Thumbnail" : { "Url": "http://www.example.com/image/481989943", "Height" : 125, "Width" : 100 }, "Animated" : false, "IDs" : [116, 943, 234, 38793] } })^^^"}
    };

    std::string s = R"^^^({ "Image": { "Width": 800, "Height" : 600, "Title" : "View from 15th Floor", "Thumbnail" : { "Url": "http://www.example.com/image/481989943", "Height" : 125, "Width" : 100 }, "Animated" : false, "IDs" : [116, 943, 234, 38793] } })^^^";

    JsonParser parser;
    BOOST_TEST(0 == JsonParser_parse(&parser, s.c_str(), s.c_str() + s.size(), check));
    BOOST_TEST(0 == expectations.size());
}


BOOST_AUTO_TEST_CASE(shall_parse_example_2)
{
    expectations = {
        {"/menu/id", "\"file\""},
        {"/menu/value", "\"File\""},
        {"/menu/popup/menuitem[0]/value", "\"New\""},
        {"/menu/popup/menuitem[0]/onclick", "\"CreateNewDoc()\""},
        {"/menu/popup/menuitem[0]", R"^^^({"value": "New", "onclick" : "CreateNewDoc()"})^^^"},
        {"/menu/popup/menuitem[1]/value", "\"Open\""},
        {"/menu/popup/menuitem[1]/onclick", "\"OpenDoc()\""},
        {"/menu/popup/menuitem[1]", R"^^^({ "value": "Open", "onclick" : "OpenDoc()" })^^^" },
        {"/menu/popup/menuitem[2]/value", "\"Close\""},
        {"/menu/popup/menuitem[2]/onclick", "\"CloseDoc()\""},
        {"/menu/popup/menuitem[2]", R"^^^({ "value": "Close", "onclick" : "CloseDoc()" })^^^" },
        {"/menu/popup/menuitem", R"^^^([ {"value": "New", "onclick" : "CreateNewDoc()"}, { "value": "Open", "onclick" : "OpenDoc()" }, { "value": "Close", "onclick" : "CloseDoc()" } ])^^^"},
        {"/menu/popup", R"^^^({ "menuitem": [ {"value": "New", "onclick" : "CreateNewDoc()"}, { "value": "Open", "onclick" : "OpenDoc()" }, { "value": "Close", "onclick" : "CloseDoc()" } ] })^^^"},
        {"/menu", R"^^^({ "id": "file", "value" : "File", "popup" : { "menuitem": [ {"value": "New", "onclick" : "CreateNewDoc()"}, { "value": "Open", "onclick" : "OpenDoc()" }, { "value": "Close", "onclick" : "CloseDoc()" } ] } })^^^"},
        {"", R"^^^({ "menu": { "id": "file", "value" : "File", "popup" : { "menuitem": [ {"value": "New", "onclick" : "CreateNewDoc()"}, { "value": "Open", "onclick" : "OpenDoc()" }, { "value": "Close", "onclick" : "CloseDoc()" } ] } } })^^^"},
    };

    std::string s = R"^^^({ "menu": { "id": "file", "value" : "File", "popup" : { "menuitem": [ {"value": "New", "onclick" : "CreateNewDoc()"}, { "value": "Open", "onclick" : "OpenDoc()" }, { "value": "Close", "onclick" : "CloseDoc()" } ] } } })^^^";

    JsonParser parser;
    BOOST_TEST(0 == JsonParser_parse(&parser, s.c_str(), s.c_str() + s.size(), check));
    BOOST_TEST(0 == expectations.size());
}

BOOST_AUTO_TEST_CASE(shall_parse_example_3)
{
    expectations = {
        {"[0]/precision", "\"zip\""},
        {"[0]/Latitude", "37.7668"},
        {"[0]/Longitude", "-122.3959"},
        {"[0]/Address", "\"\""},
        {"[0]/City", "\"SAN FRANCISCO\""},
        {"[0]/State", "\"CA\""},
        {"[0]/Zip", "\"94107\""},
        {"[0]/Country", "\"US\""},
        {"[0]", R"^^^({ "precision": "zip", "Latitude":  37.7668, "Longitude": -122.3959, "Address": "", "City": "SAN FRANCISCO", "State": "CA", "Zip": "94107", "Country": "US" })^^^" },
        {"[1]/precision", "\"zip\""},
        {"[1]/Latitude", "37.371991"},
        {"[1]/Longitude", "-122.026020"},
        {"[1]/Address", "\"\""},
        {"[1]/City", "\"SUNNYVALE\""},
        {"[1]/State", "\"CA\""},
        {"[1]/Zip", "\"94085\""},
        {"[1]/Country", "\"US\""},
        {"[1]", R"^^^({ "precision": "zip", "Latitude": 37.371991, "Longitude": -122.026020, "Address": "", "City": "SUNNYVALE", "State": "CA", "Zip": "94085", "Country":"US" })^^^" },
        {"", R"^^^([ { "precision": "zip", "Latitude":  37.7668, "Longitude": -122.3959, "Address": "", "City": "SAN FRANCISCO", "State": "CA", "Zip": "94107", "Country": "US" }, { "precision": "zip", "Latitude": 37.371991, "Longitude": -122.026020, "Address": "", "City": "SUNNYVALE", "State": "CA", "Zip": "94085", "Country":"US" } ])^^^"},
    };

    std::string s = R"^^^([ { "precision": "zip", "Latitude":  37.7668, "Longitude": -122.3959, "Address": "", "City": "SAN FRANCISCO", "State": "CA", "Zip": "94107", "Country": "US" }, { "precision": "zip", "Latitude": 37.371991, "Longitude": -122.026020, "Address": "", "City": "SUNNYVALE", "State": "CA", "Zip": "94085", "Country":"US" } ])^^^";

    JsonParser parser;
    BOOST_TEST(0 == JsonParser_parse(&parser, s.c_str(), s.c_str() + s.size(), check));
    BOOST_TEST(0 == expectations.size());
}

BOOST_AUTO_TEST_CASE(shallNotCrashIfKeysAreLong)
{
    std::string s = R"^^^({ "veryLongStringToOverloadKey_veryLongStringToOverloadKeyveryLongStringToOverloadKey_veryLongStringToOverloadKey_veryLongStringToOverloadKey_veryLongStringToOverloadKey_veryLongStringToOverloadKey_veryLongStringToOverloadKey_veryLongStringToOverloadKey": { "someAnotherVeryLongStringToOveloadBuffer_someAnotherVeryLongStringToOveloadBuffer_someAnotherVeryLongStringToOveloadBuffer_someAnotherVeryLongStringToOveloadBuffer_someAnotherVeryLongStringToOveloadBuffer" : { "andYetAnotherVeryLongStringJustToMakeThingHard_andYetAnotherVeryLongStringJustToMakeThingHard" : "value" } } })^^^";

    JsonParser parser;
    BOOST_TEST(1 == JsonParser_parse(&parser, s.c_str(), s.c_str() + s.size(), doNothing));
}

BOOST_AUTO_TEST_CASE(shallNotCrashForDeepJson)
{
    std::string s = R"^^^({ "a": { "b": { "c": { "d": { "e": { "f": { "g": { "h" : { "i" : { "j": { "k": { "l": { "m": { "n": { "o": { "p" :{ "r": { "s": { "t": { "u" : { "w": { "y" : { "z": { "aa" : { "ab": {} } } } } } } } } } } } } } } } } } } } } } } } } })^^^";

    JsonParser parser;
    BOOST_TEST(1 == JsonParser_parse(&parser, s.c_str(), s.c_str() + s.size(), doNothing));
}