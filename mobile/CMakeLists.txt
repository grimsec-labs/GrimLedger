qt_add_executable(GrimLedgerMobile
    main.cpp
    GrimVaultController.cpp
    GrimVaultController.h
    ${CMAKE_SOURCE_DIR}/android/cpp/android_jni_bridge.cpp
    mobile.qrc
)

target_include_directories(GrimLedgerMobile PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/android/cpp
)

target_link_libraries(GrimLedgerMobile PRIVATE
    grimledger_core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

set_target_properties(GrimLedgerMobile PROPERTIES
    OUTPUT_NAME "GrimLedger"
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/android
)
