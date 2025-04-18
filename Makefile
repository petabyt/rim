test: all
	build/nim_demo

test_differ:
	cmake --build build -t test_differ && build/test_differ

all:
	cmake --build build
