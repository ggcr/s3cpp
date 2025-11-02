#ifndef S3CPP_FOO
#define S3CPP_FOO

#include <string>

class Foo {
public:
	Foo() = delete;
	Foo(std::string k, int v) : key(std::move(k)), value(v) {};
private:
  int value;
  std::string key;
};

#endif
