# CMakeList.txt : projet CMake pour ia-chess, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.10)

# Activez Rechargement à chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ia-chess")

# Ajoutez une source à l'exécutable de ce projet.
add_executable (ia-chess "ia-chess.cpp" "ia-chess.h" "src/frameCapture.h" "src/frameCapture.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ia-chess PROPERTY CXX_STANDARD 20)
endif()

# TODO: Ajoutez des tests et installez des cibles si nécessaire.
