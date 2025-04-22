test: all
	build/demo

test_differ:
	cmake --build build -t test_differ && build/test_differ

all:
	cmake --build build

build:
	cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
