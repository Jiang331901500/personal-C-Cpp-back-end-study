# Notice
Before compiling the demo, you must install [libevent](https://github.com/libevent/libevent) first.

When linking the program with libevent, you should add option `-levent`

Example:
```
gcc ev_timer_sig.c -o evtest -levent
```