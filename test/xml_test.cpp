#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <s3cpp/xml.h>

TEST(XML, XMLBasicTag) {
    // <Bucket>Name</Bucket>
    auto parser = XMLParser();
    auto XMLValues = parser.parse("<Bucket>Name</Bucket>");
}

TEST(XML, XMLNestedBasicTag) {
    // <Session>
    //    <Bucket>Name</Bucket>
    // </Session>
    auto parser = XMLParser();
    auto XMLValues = parser.parse("<Session><Bucket>Name</Bucket></Session>");
}

TEST(XML, XMLNestedNestedBasicTag) {
    // <Nesting>
    //    <Session>
    //       <Bucket>Name</Bucket>
    //    </Session>
    // </Nesting>
    auto parser = XMLParser();
    auto XMLValues = parser.parse("<Nesting><Session><Bucket>Name</Bucket></Session></Nesting>");
}

TEST(XML, XMLInvalidClosingTag) {
    // <Session>
    //    <Bucket>Name</Bucket>
    // </Invalid>
    auto parser = XMLParser();
    EXPECT_ANY_THROW(parser.parse("<Session><Bucket>Name</Bucket></Invalid>"));
}

TEST(XML, XMLInvalidIncompleteTag) {
    // <Session>
    //    <Bucket>Name</Bucket>
    // <Invalid>
    auto parser = XMLParser();
    EXPECT_ANY_THROW(parser.parse("<Session><Bucket>Name</Bucket><Invalid>"));
}
