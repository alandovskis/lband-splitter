.PHONY: build run test clean

build:
	conan install . --output-folder build --build=missing
	cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
	cmake --build build

run: build
	./build/splitter

test: build
	cd build && ctest --output-on-failure

clean:
	rm -rf build
