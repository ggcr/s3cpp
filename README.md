# s3cpp

### Build

```bash
$ mkdir build
$ cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build/
```

### Run tests

```bash
$ ./build/tests [--gtest_filter='TESTSUITE.*']

# to see all available test suites
$ ./build/tests --gtest_list_tests
```
