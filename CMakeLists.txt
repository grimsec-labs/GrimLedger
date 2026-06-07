cmake_minimum_required(VERSION 3.21)
project(GrimLedger VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# --- libsodium (Argon2id + XChaCha20-Poly1305) ---
include(FetchContent)
FetchContent_Declare(
    libsodium
    GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
    GIT_TAG        1.0.20-RELEASE
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(libsodium)
if(NOT libsodium_POPULATED)
    FetchContent_Populate(libsodium)
endif()

set(SODIUM_SRC_ROOT ${libsodium_SOURCE_DIR}/src/libsodium)
file(GLOB_RECURSE SODIUM_SOURCES CONFIGURE_DEPENDS
    "${SODIUM_SRC_ROOT}/*.c"
)

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

# --- SQLite amalgamation ---
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

set(SOURCES
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
    src/security/CryptoManager.cpp
    src/security/PasswordManager.cpp
    src/security/VaultSession.cpp
    src/storage/Database.cpp
    src/storage/DbTransaction.cpp
    src/storage/NoteRepository.cpp
    src/storage/CredentialRepository.cpp
    src/storage/AttachmentRepository.cpp
    src/storage/VaultRepository.cpp
    src/search/SearchEngine.cpp
    src/utils/TimeUtils.cpp
    src/utils/SecureStringUtils.cpp
    src/utils/Theme.cpp
    src/utils/ImageSanitizer.cpp
    src/utils/PasswordGenerator.cpp
    src/utils/ClipboardUtils.cpp
    src/utils/AppSettings.cpp
    src/utils/DialogUtils.cpp
    src/bridge/OriginMatcher.cpp
    src/bridge/BridgeAuth.cpp
    src/bridge/CredentialBridgeServer.cpp
    src/utils/SqliteUtils.cpp
    src/markdown/MarkdownRenderer.cpp
    src/markdown/SyntaxHighlighter.cpp
)

set(HEADERS
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
    src/security/CryptoManager.h
    src/security/PasswordManager.h
    src/security/VaultSession.h
    src/storage/Database.h
    src/storage/DbTransaction.h
    src/storage/NoteRepository.h
    src/storage/CredentialRepository.h
    src/storage/AttachmentRepository.h
    src/storage/VaultRepository.h
    src/models/Note.h
    src/models/Credential.h
    src/models/Folder.h
    src/models/Tag.h
    src/search/SearchEngine.h
    src/utils/TimeUtils.h
    src/utils/SecureStringUtils.h
    src/utils/Theme.h
    src/utils/ImageSanitizer.h
    src/utils/PasswordGenerator.h
    src/utils/ClipboardUtils.h
    src/utils/AppSettings.h
    src/utils/DialogUtils.h
    src/bridge/OriginMatcher.h
    src/bridge/BridgeAuth.h
    src/bridge/CredentialBridgeServer.h
    src/models/CredentialSummary.h
    src/utils/RepositoryResult.h
    src/utils/SqliteUtils.h
    src/markdown/MarkdownRenderer.h
    src/markdown/SyntaxHighlighter.h
)

set(GRIMLEDGER_RESOURCES
    resources/resources.qrc
)
if(WIN32)
    list(APPEND GRIMLEDGER_RESOURCES resources/grimledger.rc)
endif()

qt_add_executable(GrimLedger
    ${SOURCES}
    ${HEADERS}
    ${GRIMLEDGER_RESOURCES}
)

target_include_directories(GrimLedger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GrimLedger PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    sqlite3_static
    sodium
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
        target_sources(${name} PRIVATE src/bridge/OriginMatcher.cpp)
        target_link_libraries(${name} PRIVATE Qt6::Core)
    elseif(${name} STREQUAL "test_sqlite_utils")
        target_sources(${name} PRIVATE src/utils/SqliteUtils.cpp)
        target_link_libraries(${name} PRIVATE sqlite3_static)
    elseif(${name} STREQUAL "test_bridge_auth")
        target_sources(${name} PRIVATE src/bridge/BridgeAuth.cpp)
        target_link_libraries(${name} PRIVATE Qt6::Core sodium)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_grimledger_test(test_origin_matcher)
add_grimledger_test(test_sqlite_utils)
add_grimledger_test(test_bridge_auth)

if(WIN32)
    set_target_properties(GrimLedger PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS GrimLedger RUNTIME DESTINATION bin)
