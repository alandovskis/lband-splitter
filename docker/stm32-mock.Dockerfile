FROM conanio/gcc12
WORKDIR /app
COPY . .
RUN cmake -S src/stm32 -B build && \
    cmake --build build
EXPOSE 50051
CMD ["./build/stm32_status_mock"]
