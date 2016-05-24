# Synthetic Memory Allocator

The program read pairs of size (in MB) and time (in seconds) from
the standard input. The program tries to allocate up to size MB in
the specified time. For instance, for the next sequence
30 30
15 60
15 30
the program 1) allocates up to 30 MB in the first 30 seconds of
execution, then 2) reduces the allocated memory down to 15 MB in
the next 60 seconds, and finally 3) maintains that during the next
30 seconds. The following figure shows the evolution of the reserved
memory.


```
30MB - |    *
       |   *    *
15MB - |  *         *****
       | *      
 0MB - |*
       --------------------
        |   |   |   |   |
       0s  30s 60s 90s 120s
```

Developed by [Eloy Romero Alcalde](https://github.com/eromero-vlc)
