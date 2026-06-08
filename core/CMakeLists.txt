# Portable GrimLedger core (crypto, storage, models) — shared by desktop and Android.

set(GRIMLEDGER_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/security/CryptoManager.cpp
    ${CMAKE_SOURCE_DIR}/src/security/PasswordManager.cpp
    ${CMAKE_SOURCE_DIR}/src/security/VaultSession.cpp
    ${CMAKE_SOURCE_DIR}/src/security/BreachCheck.cpp
    ${CMAKE_SOURCE_DIR}/src/security/ChamberManager.cpp
    ${CMAKE_SOURCE_DIR}/src/security/SecurityChronicle.cpp
    ${CMAKE_SOURCE_DIR}/src/security/SecretScanner.cpp
    ${CMAKE_SOURCE_DIR}/src/security/RedactionStudio.cpp
    ${CMAKE_SOURCE_DIR}/src/storage/Database.cpp
    ${CMAKE_SOURCE_DIR}/src/storage/DbTransaction.cpp
    ${CMAKE_SOURCE_DIR}/src/storage/NoteRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/storage/CredentialRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/storage/AttachmentRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/storage/VaultRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/storage/SealedBlockRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/models/FillTrustLevel.cpp
    ${CMAKE_SOURCE_DIR}/src/models/ChamberId.cpp
    ${CMAKE_SOURCE_DIR}/src/import/CredentialImport.cpp
    ${CMAKE_SOURCE_DIR}/src/share/GrimShare.cpp
    ${CMAKE_SOURCE_DIR}/src/search/SearchEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/search/SemanticSearch.cpp
    ${CMAKE_SOURCE_DIR}/src/markdown/MarkdownRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/markdown/SyntaxHighlighter.cpp
    ${CMAKE_SOURCE_DIR}/src/markdown/DiagramRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/graph/KnowledgeGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/runbook/RunbookParser.cpp
    ${CMAKE_SOURCE_DIR}/src/runbook/RunbookSessionRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/bridge/OriginMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/bridge/BridgeAuth.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/TotpGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/VaultHealth.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/TimeUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/SecureStringUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/PasswordGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/SqliteUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/ImageSanitizer.cpp
    ${CMAKE_SOURCE_DIR}/src/security/PlatformBiometricUnlock.cpp
)

if(WIN32 AND NOT ANDROID)
    list(APPEND GRIMLEDGER_CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/security/WindowsHelloUnlock.cpp
    )
endif()

if(ANDROID)
    list(APPEND GRIMLEDGER_CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/security/AndroidBiometricUnlock.cpp
    )
endif()

add_library(grimledger_core STATIC ${GRIMLEDGER_CORE_SOURCES})
# AUTOMOC breaks on Windows when CMake lives under "Program Files" (unquoted path).
set_target_properties(grimledger_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
qt_wrap_cpp(GRIMLEDGER_CORE_MOC
    ${CMAKE_SOURCE_DIR}/src/security/VaultSession.h
    ${CMAKE_SOURCE_DIR}/src/markdown/SyntaxHighlighter.h
)
target_sources(grimledger_core PRIVATE ${GRIMLEDGER_CORE_MOC})
target_include_directories(grimledger_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(grimledger_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    sqlite3_static
    sodium
)

if(ANDROID)
    target_compile_definitions(grimledger_core PUBLIC GRIMLEDGER_ANDROID=1)
endif()
