cmake_minimum_required(VERSION 3.1.0)
project(thirdparty)



# sources to be compiled
set(SOURCE_FILES
        xmlParser/xmlParser.cpp
        textDetect/erfilter.cpp
        LBP/helper.cpp
        LBP/lbp.cpp
        mser/mser2.cpp
        )

# pack objects to static library
add_library(thirdparty STATIC ${SOURCE_FILES})
