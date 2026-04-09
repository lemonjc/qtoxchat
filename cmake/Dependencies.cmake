find_package(OpenSSL REQUIRED)

if(MSVC)
    find_package(unofficial-sodium CONFIG REQUIRED)
else()
    find_package(libsodium CONFIG REQUIRED)
endif()

find_package(Opus CONFIG REQUIRED)
find_package(unofficial-libvpx CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
