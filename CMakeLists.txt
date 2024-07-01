# CMakeList.txt: projekt CMake dla elementu JsonParser, dołącz źródło i zdefiniuj
# tutaj logikę specyficzną dla projektu.
#
cmake_minimum_required (VERSION 3.8)

find_package(Boost)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()


project ("JsonParser")

# Dodaj źródło do pliku wykonywalnego tego projektu.
add_executable (JsonParser "JsonParser.cpp" "JsonParser.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET JsonParser PROPERTY CXX_STANDARD 20)
endif()

# TODO: Dodaj testy i zainstaluj elementy docelowe w razie potrzeby.
