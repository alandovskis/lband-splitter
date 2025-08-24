.PHONY: configure build run test clean

configure:
	conan install . --output-folder build --build=missing
	cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake

build: configure
	cmake --build build

run: build
	./build/splitter

test: build
	cd build && ctest --output-on-failure

clean:
	rm -rf build
