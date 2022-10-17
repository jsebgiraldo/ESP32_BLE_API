# Edit following two lines to set component requirements (see docs)



file(GLOB srcs
        "*.c"
        "src/*.c"
        )

set(dirs
        "inc"
        )

idf_component_register(
        SRCS ${srcs}
        INCLUDE_DIRS ${dirs}
        )