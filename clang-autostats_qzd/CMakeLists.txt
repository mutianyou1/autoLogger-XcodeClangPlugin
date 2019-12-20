
if( NOT MSVC ) # MSVC mangles symbols differently
  if( NOT LLVM_REQUIRES_RTTI )
    if( NOT LLVM_REQUIRES_EH )
      set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ClangAutoStats.exports)
    endif()
  endif()
endif()

add_llvm_library(ClangAutoStats_qzd MODULE ClangAutoStats.cpp PLUGIN_TOOL clang)
if(LLVM_ENABLE_PLUGINS AND (WIN32 OR CYGWIN))
  target_link_libraries(ClangAutoStats_qzd ${cmake_2_8_12_PRIVATE}
    LLVMSupport
    clangAST
    clangBasic
    clangDriver
    clangFormat
    clangLex
    clangParse
    clangSema
    clangFrontend
    clangTooling
    clangToolingCore
    clangRewrite
    clangRewriteFrontend
    )
endif()
