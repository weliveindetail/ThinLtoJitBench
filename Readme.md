# ThinLtoJitBench

Benchmark the speedup gained by exploiting extra information from ThinLTO module summaries in a LLVM OrcJIT compiler. This suite is in a very early state and currently only covers load-time benchmarks. Tested on MacOS and LLVM master from Sunday, October 14th 2018.

```
$ export PROJECT_ROOT=`pwd`

# Checkout llvm and clang
$ git clone https://github.com/llvm-mirror/llvm.git
$ cd llvm
$ git reset --hard aa8c49dafa1
$ cd tools
$ git clone https://github.com/llvm-mirror/clang.git
$ cd clang
$ git reset --hard e3de7bb263

# Build required parts of llvm and clang
$ cd $PROJECT_ROOT
$ mkdir llvm-relwithdebinfo
$ cd llvm-relwithdebinfo
$ cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_TARGETS_TO_BUILD=host -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_DOCS=OFF $PROJECT_ROOT/llvm
$ ninja clang
$ ninja llvm-as
$ ninja llvm-dis
$ ninja OrcJITTests

# Checkout benchmark suite
$ cd $PROJECT_ROOT
$ git clone https://github.com/weliveindetail/ThinLtoJitBench
$ cd ThinLtoJitBench
$ git submodule update --init --recursive

# Generate benchmark inputs
$ cd $PROJECT_ROOT
$ cd ThinLtoJitBench
$ export LLVM_SOURCE_DIR=$PROJECT_ROOT/llvm
$ export LLVM_BINARY_DIR=$PROJECT_ROOT/llvm-relwithdebinfo
$ export SDK_ROOT=`xcode-select -p`/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk
$ ln -s -f `xcode-select -p`/Toolchains/XcodeDefault.xctoolchain/usr/include/c++ llvm-relwithdebinfo/include
$ ./bitcode_generate.sh $LLVM_SOURCE_DIR/tools/clang/lib/CodeGen

# Build and run benchmarks
$ cd $PROJECT_ROOT
$ mkdir ThinLtoJitBench-xcode
$ cd ThinLtoJitBench-xcode
$ cmake -GXcode -DLLVM_DIR=$PROJECT_ROOT/llvm-relwithdebinfo/lib/cmake/llvm $PROJECT_ROOT/ThinLtoJitBench
$ xcodebuild -configuration Release
$ ./Release/ThinLtoJitBench --benchmark_repetitions=10 --benchmark_report_aggregates_only=true

# Sample results on my machine
Collected 55 input files

2018-10-16 15:22:56
Running ./Release/ThinLtoJitBench
Run on (4 X 3500 MHz CPU s)
CPU Caches:
  L1 Data 32K (x2)
  L1 Instruction 32K (x2)
  L2 Unified 262K (x2)
  L3 Unified 4194K (x1)
-----------------------------------------------------------------------
Benchmark                                Time           CPU Iterations
-----------------------------------------------------------------------
BM_FrontloadEager_mean          1957157791 ns 1941012500 ns          1
BM_FrontloadEager_median        1952136629 ns 1937263000 ns          1
BM_FrontloadEager_stddev          78667674 ns   73155684 ns          1
BM_FrontloadLazy_mean            588991602 ns  543883100 ns          1
BM_FrontloadLazy_median          544827919 ns  528369000 ns          1
BM_FrontloadLazy_stddev          103742572 ns   41503101 ns          1
BM_FrontloadSummaryBased_mean    366003920 ns  362598450 ns          2
BM_FrontloadSummaryBased_median  367777140 ns  364415250 ns          2
BM_FrontloadSummaryBased_stddev   20190528 ns   18496763 ns          2
```
