# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS         # list the source files of this component
    SRC_DIRS ../c_fonts ../c_elements ../c_pages ../c_graphics ../main
    INCLUDE_DIRS include # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES fatfs sdmmc  # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
