# Helang++ 基于LLVM的编译型赛博编程语言

太酷了！满足了我对编译器的所有幻想。

环境要求：

- windows 操作系统（目前不支持其它操作系统）
- 编译并安装llvm以及clang(由于本人底下的技术力，不得不用clang作为连接器)
- cmake 

如何从源码构建并安装llvm可以参考https://llvm.org/docs/CMake.html

在构建项目时需要指出llvm的安装路径

```
> mkdir build
> cd build
> cmake -DLLVM_PATH=[Path to your llvm install root directory] ..
> cmake --config Release --build .
```

