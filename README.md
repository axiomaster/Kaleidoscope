llvm.org/docs/tutorial/index.html

[用LLVM开发新语言](https://llvm-tutorial-cn.readthedocs.io/en/latest/)

- 编译失败

```
[ 50%] Linking CXX executable Kaleidoscope
CMakeFiles/Kaleidoscope.dir/Kaleidoscope.cpp.o: In function `main':
/mnt/d/code/Kaleidoscope/Kaleidoscope/Kaleidoscope.cpp:543: undefined reference to `llvm::Module::dump() const'
clang: error: linker command failed with exit code 1 (use -v to see invocation)
Kaleidoscope/CMakeFiles/Kaleidoscope.dir/build.make:83: recipe for target 'Kaleidoscope/Kaleidoscope' failed
make[2]: *** [Kaleidoscope/Kaleidoscope] Error 1
CMakeFiles/Makefile2:90: recipe for target 'Kaleidoscope/CMakeFiles/Kaleidoscope.dir/all' failed
make[1]: *** [Kaleidoscope/CMakeFiles/Kaleidoscope.dir/all] Error 2
Makefile:83: recipe for target 'all' failed
make: *** [all] Error 2
```

[https://stackoverflow.com/questions/46367910/llvm-5-0-linking-error-with-llvmmoduledump](https://stackoverflow.com/questions/46367910/llvm-5-0-linking-error-with-llvmmoduledump)

- visual studio wsl cmake intellisense 头文件不识别

这个问题没有搞定，把```llvm```对应的头文件放在```/usr/include/```目录下，并用vs同步头文件。这也说明，intellisense使用的头文件是同步在windows下的头文件，而非wsl中对应的目录。