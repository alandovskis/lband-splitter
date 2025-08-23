# L-Band Splitter Controller

This directory contains sample C++ code and an Angular component stub for a 32-port L-band splitter/combiner.

The C++ demo computes the center frequency of a synthetic signal using an FFT and stores the result in a `PortState` object.

Build and run using CMake:

```
mkdir build
cd build
cmake ..
cmake --build .
./splitter
```

The Angular component (`web/port-status.component.*`) illustrates how a frontend might interact with the backend.
