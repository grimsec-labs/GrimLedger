# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-src")
  file(MAKE_DIRECTORY "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-src")
endif()
file(MAKE_DIRECTORY
  "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-build"
  "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-subbuild/libsodium-populate-prefix"
  "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-subbuild/libsodium-populate-prefix/tmp"
  "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp"
  "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-subbuild/libsodium-populate-prefix/src"
  "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Dev/repos/GrimLedger/build-android/_deps/libsodium-subbuild/libsodium-populate-prefix/src/libsodium-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
