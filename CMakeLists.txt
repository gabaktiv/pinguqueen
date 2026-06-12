#Chatgpt generierte strenge Cmake

cmake_minimum_required(VERSION 4.2.2)
project(pinguqueen CXX)

# 1. C++ Standard erzwingen
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Deaktiviert Compiler-spezifische Erweiterungen für echtes Standard-C++

# 2. Das Executable erstellen
add_executable(pinguqueen src/main.cpp
        src/radix-trie/radix-trie.cpp
        src/radix-trie/node.cpp)

# 3. Extrem strenge Compiler-Flags NUR für Clang (und GCC) definieren
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(pinguqueen PRIVATE
            -Wall            # Aktiviert die wichtigsten Warnungen
            -Wextra          # Aktiviert zusätzliche, noch strengere Warnungen
            -Wpedantic       # Erzwingt strikte Einhaltung des ISO C++ Standards
            -Werror          # Macht ALLE Warnungen zu fatalen Fehlern (Kompilierung bricht ab)

            # Spezifische Clang-Zusatzabsicherungen:
            -Wshadow         # Warnt, wenn eine Variable eine andere verdeckt
            -Wnon-virtual-dtor # Warnt bei fehlenden virtuellen Destruktoren in Basisklassen
            -Wold-style-cast # Warnt bei C-Style Casts (erzwingt static_cast, reinterpret_cast etc.)
            -Wcast-align     # Warnt bei potenziell gefährlichen Pointer-Casts
            -Wunused         # Warnt vor ungenutzten Variablen/Funktionen
            -Woverloaded-virtual # Warnt, wenn eine abgeleitete Funktion eine virtuelle Funktion versehentlich versteckt
            -Wconversion     # Warnt bei impliziten Typkonversionen mit Datenverlust (z.B. double zu int)
            -Wsign-conversion # Warnt bei Vorzeichenwechseln (z.B. int zu unsigned int)
            -Wnull-dereference # Warnt, wenn ein Nullpointer dereferenziert werden könnte
    )
endif()