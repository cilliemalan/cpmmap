# Cross Platform Memory Mapped Files
A cross platform C++ header-only library for mapping files into memory.
```
    // simply include the header
    #include <cpmmap.h>
    // everithing is in the cpmmap namespace
    using namespace cpmmap;

    // map an existing file read-only
    mapped_file m("thefile.bin");
    
    // two ways to work with the data
    char* pointer = m.get();
    char tenthchar = m[10];

    // you also have the size
    unsigned long long numbyts = m.size();


    // map an existing file read/write
    size_t size = 1024;
    mapped_file m("thefile.bin", true, size);


    // create a new file
    auto m = mapped_file::create("newfile.bin", size);


    // resize a file mapped with read/write
    m.resize(newsize);


    // map a file in rw and shared mode,
    // allowing other processes to read the file.
    mapped_file m("thefile.bin", true, true);

```C++

##Notes
- "Cross platform" means Windows and Linux. I don't have a Mac.
- Throws exceptions when stuff goes wrong.
- Don't use `new` and `delete`. The file is unmapped when it goes out of scope.
- `mapped_file` cannot be copied but it can be moved.
- It has a `shared` option in the constructor, allowing other processes and operations
  to read the file using normal I/O or map the file. Writing a file using normal I/O
  doesn't seem to be possible if the file is mapped in any way.
- If the same file is mapped multiple times, it will be mapped in seperate locations.
  If the memory is changed in one it will reflect in the other.

##Limitations
- Has no thread safety. If you're doing stuff that would need thread safety, think
  about what you're doing a bit more and then implement your own synchronization.
- Pointers into the mapped block will be invalidated when the block is resized.
- Doesn't implement iterators. If you need them for some reason use `&[0]` and `&[size()]`.


#LICENSE
See [License]