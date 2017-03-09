
## Compiling

On Mac, compile using:

```
clang++ -std=c++11 jason-write.cc -o jason-write `pkg-config --cflags --libs cairomm-1.0`
```

On Linux, compile using

```
g++-4.9 -std=c++11 jason-write.cc -o jason-write `pkg-config --cflags --libs cairomm-1.0`
```
