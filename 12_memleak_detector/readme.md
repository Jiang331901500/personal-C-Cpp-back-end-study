# use in your code
Just define `__MALLOC_DEBUG` and include the header file `memleak_detector.h` after all other header in the source file. Example:
```c
#include <unistd.h> // other headers....

#define __MALLOC_DEBUG
#include "memleak_detector.h"
```

# compile
```
gcc *.c -o test
```

# run
Before running the demo, make sure there is a diretory named "mem". The leak infomation will be reserved in "mem".