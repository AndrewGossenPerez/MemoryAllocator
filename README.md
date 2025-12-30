This is an OS backed memory allocator, using mmap for heap creation. 
It allows you to create an RAII-managed heap buffer of certain size in bytes (automatically aligned), and manage memory within it.
The standard malloc and free is replaced with alloc and release respectively. 

To do list : 
First-fit and Best-fit algorithm options ( You decide which to use ),
Benchmarking against the standard malloc and free, thread-safety 
