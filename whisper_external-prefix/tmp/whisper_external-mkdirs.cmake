# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/ahishmahesh/Personal/Programming/cpp/third_party/whisper.cpp")
  file(MAKE_DIRECTORY "/Users/ahishmahesh/Personal/Programming/cpp/third_party/whisper.cpp")
endif()
file(MAKE_DIRECTORY
  "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix/src/whisper_external-build"
  "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix"
  "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix/tmp"
  "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix/src/whisper_external-stamp"
  "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix/src"
  "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix/src/whisper_external-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix/src/whisper_external-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ahishmahesh/Personal/Programming/cpp/agent-notes-backend/whisper_external-prefix/src/whisper_external-stamp${cfgdir}") # cfgdir has leading slash
endif()
