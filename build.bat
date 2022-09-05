if not exist build (
	mkdir build
	echo "build directory has been created"
)
cmake -B build . -DLLVM_PATH=E:/llvm/lib

if "%1" == "s" goto start_ide
if "%1" == "start" goto start_ide
if "%1" == "-s" goto start_ide
if "%1" == "--start" goto start_ide
goto end
:start_ide
start build\helang.sln
:end