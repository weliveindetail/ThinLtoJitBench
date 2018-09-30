#!/bin/sh
#set -x

# Configure with:
#
# $ export LLVM_SOURCE_DIR=/path/to/llvm
# $ export LLVM_BINARY_DIR=/path/to/ninja-relwithdebinfo
# $ export SYS_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk

if [ -e "${LLVM_BINARY_DIR}/bin/clang" ]
then
    echo "Found clang: ${LLVM_BINARY_DIR}/bin/clang"
else
    echo "Failed to find clang: ${LLVM_BINARY_DIR}/bin/clang"
    echo "Please export LLVM_BINARY_DIR=/path/to/bin/clang"
    exit 1
fi

if [ -e "${LLVM_BINARY_DIR}/bin/llvm-dis" ]
then
    echo "Found llvm-dis: ${LLVM_BINARY_DIR}/bin/llvm-dis"
else
    echo "Failed to find llvm-dis: ${LLVM_BINARY_DIR}/bin/llvm-dis"
    echo "Please export LLVM_BINARY_DIR=/path/to/bin/llvm-dis"
    exit 1
fi

if [ -e "${LLVM_BINARY_DIR}/bin/llvm-as" ]
then
    echo "Found llvm-as: ${LLVM_BINARY_DIR}/bin/llvm-as"
else
    echo "Failed to find llvm-as: ${LLVM_BINARY_DIR}/bin/llvm-as"
    echo "Please export LLVM_BINARY_DIR=/path/to/bin/llvm-as"
    exit 1
fi

if [ $# -gt 0 ]; then
    BITCODE_SOURCE_DIR="$1"
else
    BITCODE_SOURCE_DIR="${LLVM_SOURCE_DIR}/tools/clang/lib/AST"
fi

if [ -e ${BITCODE_SOURCE_DIR} ]
then
    echo "Found input directory: ${BITCODE_SOURCE_DIR}"
else
    echo "Failed to find input directory: ${BITCODE_SOURCE_DIR}"
    echo "Please export LLVM_SOURCE_DIR=/path/to/tools/clang/lib/AST"
    exit 1
fi

BITCODE_DEST_DIR=bitcode
if [ -e ${BITCODE_DEST_DIR} ]
then
    echo "Found output directory: ${BITCODE_DEST_DIR}"
else
    mkdir ${BITCODE_DEST_DIR}
    echo "Created output directory: ${BITCODE_DEST_DIR}"
fi

BITCODE_TMP_DIR=bitcode/tmp
if [ -e ${BITCODE_TMP_DIR} ]
then
    echo "Found tmp directory: ${BITCODE_TMP_DIR}"
else
    mkdir ${BITCODE_TMP_DIR}
    echo "Created tmp directory: ${BITCODE_TMP_DIR}"
fi

if [ -z ${SYS_ROOT} ]
then
    SYS_ROOT="/"
    echo "Using default SYS_ROOT: ${SYS_ROOT}"
else
    echo "SYS_ROOT: ${SYS_ROOT}"
fi

for f in ${BITCODE_SOURCE_DIR}/*.c* ; do
    # At the moment clang cannot write BC files with module summaries directly.
    # The workaround is to compile to LTO .o files, then disassemble to .ll and assemble to .bc files.
    BASE_NAME=`basename "${f%.c*}"`
    echo "Compile: $f -> ${BASE_NAME}.o -> ${BASE_NAME}.ll -> ${BITCODE_DEST_DIR}/${BASE_NAME}.bc"

    ${LLVM_BINARY_DIR}/bin/clang -c -flto=thin \
        -I${BITCODE_SOURCE_DIR} \
        -I${LLVM_SOURCE_DIR}/include \
        -I${LLVM_SOURCE_DIR}/tools/clang/include \
        -I${LLVM_BINARY_DIR}/include \
        -I${LLVM_BINARY_DIR}/tools/clang/include/ \
        -isysroot ${SYS_ROOT} \
        -o "${BITCODE_TMP_DIR}/${BASE_NAME}.o" "$f"

    # Abort if we failed to write the expected output.
    if [[ ! -e "${BITCODE_TMP_DIR}/${BASE_NAME}.o" ]]
    then
        echo "Failed to produce intermediate output: ${BITCODE_TMP_DIR}/${BASE_NAME}.o"
        echo "Abort"
        exit 1
    fi

    ${LLVM_BINARY_DIR}/bin/llvm-dis "${BITCODE_TMP_DIR}/${BASE_NAME}.o" -o "${BITCODE_TMP_DIR}/${BASE_NAME}.ll"
    ${LLVM_BINARY_DIR}/bin/llvm-as "${BITCODE_TMP_DIR}/${BASE_NAME}.ll" -o "${BITCODE_DEST_DIR}/${BASE_NAME}.bc"
done
