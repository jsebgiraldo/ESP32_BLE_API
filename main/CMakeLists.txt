# Edit following two lines to set component requirements (see docs)



file(GLOB srcs
        "*.c"
        "src/*.c"
        "src/BLE/*.c"
        )

set(dirs
        "inc"
        "inc/BLE"
        )

set(files
        "../doc/web/app.css"
        "../doc/web/app.js"
        "../doc/web/favicon.ico"
        "../doc/web/index.html"
        "../doc/web/jquery-3.3.1.min.js"
        )

idf_component_register(
        SRCS ${srcs}
        INCLUDE_DIRS ${dirs}
        EMBED_FILES  ${files}
        )