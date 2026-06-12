cmake_minimum_required(VERSION 3.21)
project(GrimLedger VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 QuickDialogs2 Network)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
endif()

# --- libsodium ---
include(FetchContent)
# GL-SEC-009: pin to the immutable commit SHA of the 1.0.20-RELEASE tag rather than
# the mutable tag name, so a re-pointed tag or compromised mirror cannot substitute
# a different libsodium into the build.
FetchContent_Declare(
    libsodium
    GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
    GIT_TAG        9511c982fb1d046470a8b42aa36556cdb7da15de  # 1.0.20-RELEASE
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(libsodium)
if(NOT libsodium_POPULATED)
    FetchContent_Populate(libsodium)
endif()

set(SODIUM_SRC_ROOT ${libsodium_SOURCE_DIR}/src/libsodium)
file(GLOB_RECURSE SODIUM_SOURCES CONFIGURE_DEPENDS "${SODIUM_SRC_ROOT}/*.c")
configure_file(
    "${libsodium_SOURCE_DIR}/builds/msvc/version.h"
    "${SODIUM_SRC_ROOT}/include/sodium/version.h"
    COPYONLY
)

add_library(sodium STATIC ${SODIUM_SOURCES})
set_target_properties(sodium PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(sodium PUBLIC "${SODIUM_SRC_ROOT}/include")
target_include_directories(sodium PRIVATE "${SODIUM_SRC_ROOT}/include/sodium")
target_compile_definitions(sodium PUBLIC SODIUM_STATIC=1)
if(MINGW)
    target_compile_definitions(sodium PRIVATE _GNU_SOURCE)
endif()
if(MSVC)
    target_compile_definitions(sodium PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# --- SQLite ---
FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
    URL_HASH SHA256=77823cb110929c2bcb0f5d48e4833b5c59a8a6e40cdea3936b99e199dbbe5784
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3)
add_library(sqlite3_static STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
set_target_properties(sqlite3_static PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(sqlite3_static PUBLIC ${sqlite3_SOURCE_DIR})
if(MSVC)
    target_compile_definitions(sqlite3_static PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(core)

if(ANDROID)
    add_subdirectory(mobile)
else()
    set(DESKTOP_SOURCES
        src/main.cpp
        src/App.cpp
        src/ui/LoginWindow.cpp
        src/ui/MainWindow.cpp
        src/ui/Sidebar.cpp
        src/ui/NoteList.cpp
        src/ui/NoteEditor.cpp
        src/ui/CredentialList.cpp
        src/ui/CredentialEditor.cpp
        src/ui/MarkdownPreview.cpp
        src/ui/SettingsWindow.cpp
        src/ui/CustomTitleBar.cpp
        src/ui/FramelessResize.cpp
        src/ui/FolderPickerDialog.cpp
        src/ui/GrimDialog.cpp
        src/ui/GrimMessageDialog.cpp
        src/ui/GrimInputDialog.cpp
        src/ui/GrimFileDialog.cpp
        src/bridge/BridgeClipCoordinator.cpp
        src/bridge/CredentialBridgeServer.cpp
        src/bridge/BridgeFillCoordinator.cpp
        src/utils/Theme.cpp
        src/utils/ClipboardUtils.cpp
        src/utils/DialogUtils.cpp
    )

    set(DESKTOP_HEADERS
        src/App.h
        src/ui/LoginWindow.h
        src/ui/MainWindow.h
        src/ui/Sidebar.h
        src/ui/NoteList.h
        src/ui/NoteEditor.h
        src/ui/CredentialList.h
        src/ui/CredentialEditor.h
        src/ui/MarkdownPreview.h
        src/ui/SettingsWindow.h
        src/ui/CustomTitleBar.h
        src/ui/FramelessResize.h
        src/ui/FolderPickerDialog.h
        src/ui/GrimDialog.h
        src/ui/GrimMessageDialog.h
        src/ui/GrimInputDialog.h
        src/ui/GrimFileDialog.h
        src/bridge/BridgeClipCoordinator.h
        src/bridge/CredentialBridgeServer.h
        src/bridge/BridgeFillCoordinator.h
        src/utils/Theme.h
        src/utils/ClipboardUtils.h
        src/utils/DialogUtils.h
    )

    set(GRIMLEDGER_RESOURCES resources/resources.qrc)
    if(WIN32)
        list(APPEND GRIMLEDGER_RESOURCES resources/grimledger.rc)
    endif()

    qt_add_executable(GrimLedger
        ${DESKTOP_SOURCES}
        ${DESKTOP_HEADERS}
        ${GRIMLEDGER_RESOURCES}
    )

    target_include_directories(GrimLedger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(GrimLedger PRIVATE
        grimledger_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
    )

    add_executable(grimledger_host
        tools/grimledger_host/main.cpp
        src/bridge/BridgeAuth.cpp
    )
    set_target_properties(grimledger_host PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    target_include_directories(grimledger_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(grimledger_host PRIVATE Qt6::Core Qt6::Network sodium)
    if(WIN32)
        set_target_properties(grimledger_host PROPERTIES WIN32_EXECUTABLE FALSE)
    endif()

    enable_testing()

    function(add_grimledger_test name)
        add_executable(${name} tests/${name}.cpp)
        set_target_properties(${name} PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
        target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        if(${name} STREQUAL "test_origin_matcher")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_sqlite_utils")
            target_sources(${name} PRIVATE src/utils/SqliteUtils.cpp)
            target_link_libraries(${name} PRIVATE sqlite3_static)
        elseif(${name} STREQUAL "test_bridge_auth")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_bridge_server")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_credential_repository")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_note_repository")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_vault_restore")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_totp_generator")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_image_sanitizer")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core Qt6::Gui)
        elseif(${name} STREQUAL "test_attachment_refs")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_stage_b")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_body_search")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_fill_trust_level")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        elseif(${name} STREQUAL "test_password_generator")
            target_link_libraries(${name} PRIVATE grimledger_core Qt6::Core)
        endif()
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    add_grimledger_test(test_origin_matcher)
    add_grimledger_test(test_sqlite_utils)
    add_grimledger_test(test_bridge_auth)
    add_grimledger_test(test_bridge_server)
    add_grimledger_test(test_credential_repository)
    add_grimledger_test(test_note_repository)
    add_grimledger_test(test_vault_restore)
    add_grimledger_test(test_totp_generator)
    add_grimledger_test(test_image_sanitizer)
    add_grimledger_test(test_attachment_refs)
    add_grimledger_test(test_stage_b)
    add_grimledger_test(test_body_search)
    add_grimledger_test(test_fill_trust_level)
    add_grimledger_test(test_password_generator)

    if(WIN32)
        set_target_properties(GrimLedger PROPERTIES WIN32_EXECUTABLE TRUE)
        target_link_libraries(GrimLedger PRIVATE crypt32)
    elseif(APPLE)
        set_target_properties(GrimLedger PROPERTIES MACOSX_BUNDLE TRUE)
    endif()

    install(TARGETS grimledger_host RUNTIME DESTINATION bin)
    if(APPLE)
        install(TARGETS GrimLedger BUNDLE DESTINATION .)
    else()
        install(TARGETS GrimLedger RUNTIME DESTINATION bin)
    endif()
endif()
