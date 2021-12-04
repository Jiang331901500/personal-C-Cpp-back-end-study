# compile:
Before compiling the source code, modify macros about Redis server ip, port and etc. according to your own environment.
```
g++ -o hiredis_apiDemo RedisConn_testDemo.cpp RedisConn.cpp -L. -lhiredis
```
# Run:
```
./hiredis_apiDemo
```

# Recompile the api lib (if needed):
```
gcc -c hiredis/*.c
ar -r libhiredis.a *.o
rm *.o
```
