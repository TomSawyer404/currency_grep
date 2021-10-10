# Currency Grep

Currency grep utility using multithread.  

# Usage

```bash
$ make clean
$ make
$ ./target/cgrep [pattern] [file]
```

First it opens a file according to argv[2], and then  
it create a `mmap()`, mapping the file to address space
of this process.  
Second, it creates some threads according to the size of
this file and the number of CPU. I devide the file into 
same sized unit for each thread.
Last, each thread uses KMP alogrithm to search pattern string.  

