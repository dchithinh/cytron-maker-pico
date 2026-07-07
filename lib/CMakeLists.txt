# Shared libraries for the Cytron Maker Pi Pico workspace live under this folder.
#
# Recommended layout for each reusable library:
#
#   lib/
#     my_library/
#       CMakeLists.txt
#       include/
#         my_library/
#           my_library.h
#       src/
#         my_library.c
#
# Then register it here with:
#   add_subdirectory(my_library)
#
# Keeping this file valid even when no shared libraries exist yet lets projects
# include the folder unconditionally from the repo root.

add_subdirectory(pico_cli)
