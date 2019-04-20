# Cross Platform Memory Mapped Files
A cross platform C++ header-only library for mapping files into memory.
Memory mapping a file means the entire file is acessible via a pointer.
```cpp
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
    mapped_file m("thefile.bin", true);


    // create a new file
    size_t size = 1024;
    auto m = mapped_file::create("newfile.bin", size);

    // force flushing changes to disk. The O/S does this
    // pretty quickly anyway, but sync blocks until it's done.
    // sync also updates the file timestamp.
    m[100] = 'A';
    m.sync();

    // resize a file mapped with read/write
    m.resize(newsize);


    // map a file in rw and shared mode,
    // allowing other processes to read the file.
    mapped_file m("thefile.bin", true, true);
```

## Notes
- "Cross platform" means Windows and Linux. I don't have a Mac.
- Throws exceptions when stuff goes wrong.
- Don't use `new` and `delete`. The file is unmapped when it goes out of scope.
- `mapped_file` cannot be copied but it can be moved (I mean C++ object
  move and copy, not file).
- It has a `shared` option in the constructor, allowing other processes and operations
  to read the file using normal I/O or map the file. Writing a file using normal I/O
  doesn't seem to be possible if the file is mapped in any way.
- If the same file is mapped multiple times, it will be mapped in seperate locations.
  If the memory is changed in one it will reflect in the other.
- Changes to a memory mapped file get automatically flushed to disk by the O/S. File caching
  also means that other processes that read the file should usually also see
  changes immediately. However, if the system fails changes may be lost. For this, when you
  call sync, it will force a flush to disk and wait for it to complete before returning.
- Changing a memory mapped file doesn't update its timestamp. However, calling sync will
  update the timestamp. The timestamp isn't changed explicitly when the file is closed.

## Limitations
- It always maps the entire file at a location chosen by the OS. It doesn't fault in
  the entire file.
- Has no thread safety. If you're doing stuff that would need thread safety, think
  about what you're doing a bit more and then implement your own synchronization.
- Pointers into the mapped block will be invalidated when the block is resized.
- Doesn't implement iterators. If you need them for some reason
  use `m.get()` and `m.get() + m.size()`.
- No copy-on-write support.


# LICENSE
See [License](License).