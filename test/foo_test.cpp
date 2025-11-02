#include <gtest/gtest.h>
#include <s3cpp/foo.h>

TEST(FooConstructors, RequiresParamsConstructor) {
	EXPECT_FALSE(std::is_default_constructible<Foo>());
	EXPECT_TRUE((std::is_constructible_v<Foo, std::string, int>));
}
