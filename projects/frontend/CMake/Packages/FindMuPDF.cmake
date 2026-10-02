# FindMuPDF.cmake
# ---------------
# Trouve la bibliothèque MuPDF et ses dépendances
#
# Ce module définit:
#  MuPDF_FOUND        - TRUE si MuPDF a été trouvé
#  MuPDF_INCLUDE_DIRS - Répertoires d'include pour MuPDF
#  MuPDF_LIBRARIES    - Bibliothèques à linker
#  MuPDF_VERSION      - Version de MuPDF (si détectable)
#
# Et crée la target importée:
#  MuPDF::MuPDF       - Target importée utilisable avec target_link_libraries
#
# Variables de contrôle:
#  MuPDF_ROOT_DIR     - Répertoire racine où chercher MuPDF
#  MuPDF_USE_STATIC   - Si TRUE, cherche les bibliothèques statiques

include(FindPackageHandleStandardArgs)

# Chemins de recherche
set(MuPDF_SEARCH_PATHS
    ${MuPDF_ROOT_DIR}
    $ENV{MUPDF_DIR}
    /usr
    /usr/local
    /opt
    /opt/local
)

# Recherche du header principal
find_path(MuPDF_INCLUDE_DIR
    NAMES mupdf/fitz.h
    PATHS ${MuPDF_SEARCH_PATHS}
    PATH_SUFFIXES include
    DOC "Répertoire d'include de MuPDF"
)

# Recherche des bibliothèques
# MuPDF utilise plusieurs libs: mupdf, mupdf-third (dépendances tierces)

# Configuration du type de bibliothèque recherchée
if(${MuPDF_USE_STATIC})
    # Forcer la recherche de bibliothèques statiques uniquement
    set(CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    set(MuPDF_LIB_NAMES mupdf)
    set(MuPDF_THIRD_LIB_NAMES mupdf-third)
else()
    # Forcer la recherche de bibliothèques dynamiques en priorité
    set(CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if(UNIX)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .so .a)
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .dll.a .a)
    endif()
    set(MuPDF_LIB_NAMES mupdf)
    set(MuPDF_THIRD_LIB_NAMES mupdf-third)
endif()

find_library(MuPDF_LIBRARY
    NAMES ${MuPDF_LIB_NAMES}
    PATHS ${MuPDF_SEARCH_PATHS}
    PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu lib/aarch64-linux-gnu lib/arm-linux-gnueabihf
    DOC "Bibliothèque principale MuPDF"
)

message(STATUS "lib: ${MuPDF_LIBRARY} from ${MuPDF_LIB_NAMES} in ${MuPDF_SEARCH_PATHS}")

find_library(MuPDF_THIRD_LIBRARY
    NAMES ${MuPDF_THIRD_LIB_NAMES}
    PATHS ${MuPDF_SEARCH_PATHS}
    PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu lib/aarch64-linux-gnu lib/arm-linux-gnueabihf
    DOC "Bibliothèque de dépendances tierces MuPDF"
)

# Restaurer les suffixes de bibliothèque
if(CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP})
    unset(CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP)
endif()

# Tentative de détection de la version
if(MuPDF_INCLUDE_DIR)
    if(EXISTS "${MuPDF_INCLUDE_DIR}/mupdf/fitz/version.h")
        file(STRINGS "${MuPDF_INCLUDE_DIR}/mupdf/fitz/version.h" 
             MuPDF_VERSION_LINE 
             REGEX "^#define[ \t]+FZ_VERSION[ \t]+\"[^\"]*\"")
        
        if(MuPDF_VERSION_LINE)
            string(REGEX REPLACE "^#define[ \t]+FZ_VERSION[ \t]+\"([^\"]*)\".*" "\\1" 
                   MuPDF_VERSION "${MuPDF_VERSION_LINE}")
        endif()
    endif()
endif()

# Liste complète des bibliothèques
set(MuPDF_LIBRARIES_LIST)
if(MuPDF_LIBRARY)
    list(APPEND MuPDF_LIBRARIES_LIST ${MuPDF_LIBRARY})
endif()
if(MuPDF_THIRD_LIBRARY)
    list(APPEND MuPDF_LIBRARIES_LIST ${MuPDF_THIRD_LIBRARY})
endif()

# Dépendances système requises par MuPDF
find_package(Threads REQUIRED)
list(APPEND MuPDF_LIBRARIES_LIST ${CMAKE_THREAD_LIBS_INIT})

# Recherche des dépendances optionnelles/requises
find_library(M_LIBRARY m)
if(M_LIBRARY)
    list(APPEND MuPDF_LIBRARIES_LIST ${M_LIBRARY})
endif()

# Gestion standard du package
find_package_handle_standard_args(MuPDF
    REQUIRED_VARS 
        MuPDF_LIBRARY 
        MuPDF_INCLUDE_DIR
    VERSION_VAR 
        MuPDF_VERSION
    FAIL_MESSAGE 
        "MuPDF non trouvé. Essayez: sudo apt-get install libmupdf-dev"
)

if(MuPDF_FOUND)
    set(MuPDF_INCLUDE_DIRS ${MuPDF_INCLUDE_DIR})
    set(MuPDF_LIBRARIES ${MuPDF_LIBRARIES_LIST})
    
    # Marquer les variables avancées
    mark_as_advanced(
        MuPDF_INCLUDE_DIR
        MuPDF_LIBRARY
        MuPDF_THIRD_LIBRARY
    )
    
    # Créer une target importée si elle n'existe pas déjà
    if(NOT TARGET MuPDF::MuPDF)
        add_library(MuPDF::MuPDF UNKNOWN IMPORTED)
        
        set_target_properties(MuPDF::MuPDF PROPERTIES
            IMPORTED_LOCATION "${MuPDF_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MuPDF_INCLUDE_DIRS}"
        )
        
        # Ajouter les bibliothèques additionnelles
        set(MuPDF_INTERFACE_LIBS)
        if(MuPDF_THIRD_LIBRARY)
            list(APPEND MuPDF_INTERFACE_LIBS ${MuPDF_THIRD_LIBRARY})
        endif()
        list(APPEND MuPDF_INTERFACE_LIBS ${CMAKE_THREAD_LIBS_INIT})
        if(M_LIBRARY)
            list(APPEND MuPDF_INTERFACE_LIBS ${M_LIBRARY})
        endif()
        
        set_property(TARGET MuPDF::MuPDF PROPERTY
            INTERFACE_LINK_LIBRARIES ${MuPDF_INTERFACE_LIBS}
        )
    endif()
    
    # Message de debug (optionnel)
    if(NOT MuPDF_FIND_QUIETLY)
        message(STATUS "MuPDF trouvé:")
        message(STATUS "  Version: ${MuPDF_VERSION}")
        message(STATUS "  Include: ${MuPDF_INCLUDE_DIRS}")
        message(STATUS "  Library: ${MuPDF_LIBRARY}")
        if(MuPDF_THIRD_LIBRARY)
            message(STATUS "  Third-party: ${MuPDF_THIRD_LIBRARY}")
        endif()
        
        # Afficher le type de bibliothèque
        if(MuPDF_LIBRARY MATCHES "\\.a$")
            message(STATUS "  Type: Statique (.a)")
        elseif(MuPDF_LIBRARY MATCHES "\\.so")
            message(STATUS "  Type: Dynamique (.so)")
        endif()
    endif()
endif()
