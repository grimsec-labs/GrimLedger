# CMake generated Testfile for 
# Source directory: D:/Dev/repos/claude_grimledger/GrimLedger
# Build directory: D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[test_origin_matcher]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_origin_matcher.exe")
set_tests_properties([=[test_origin_matcher]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;189;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_sqlite_utils]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_sqlite_utils.exe")
set_tests_properties([=[test_sqlite_utils]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;190;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_bridge_auth]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_bridge_auth.exe")
set_tests_properties([=[test_bridge_auth]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;191;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_bridge_server]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_bridge_server.exe")
set_tests_properties([=[test_bridge_server]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;192;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_credential_repository]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_credential_repository.exe")
set_tests_properties([=[test_credential_repository]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;193;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_note_repository]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_note_repository.exe")
set_tests_properties([=[test_note_repository]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;194;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_vault_restore]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_vault_restore.exe")
set_tests_properties([=[test_vault_restore]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;195;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_totp_generator]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_totp_generator.exe")
set_tests_properties([=[test_totp_generator]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;196;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_image_sanitizer]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_image_sanitizer.exe")
set_tests_properties([=[test_image_sanitizer]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;197;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_attachment_refs]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_attachment_refs.exe")
set_tests_properties([=[test_attachment_refs]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;198;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
add_test([=[test_stage_b]=] "D:/Dev/repos/claude_grimledger/GrimLedger/build-desktop-test/test_stage_b.exe")
set_tests_properties([=[test_stage_b]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;186;add_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;199;add_grimledger_test;D:/Dev/repos/claude_grimledger/GrimLedger/CMakeLists.txt;0;")
subdirs("core")
