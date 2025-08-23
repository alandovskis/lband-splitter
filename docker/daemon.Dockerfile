FROM conanio/gcc12
WORKDIR /app
COPY . .
RUN conan profile detect --force && \
    conan install . --output-folder build --build=missing && \
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake && \
    cmake --build build --target monitor_daemon
EXPOSE 9002
CMD ["./build/monitor_daemon"]
