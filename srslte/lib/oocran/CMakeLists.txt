file(GLOB SOURCES "*.c")
add_library(srslte_oocran OBJECT ${SOURCES})

# target_link_libraries(srslte_oocran ${PYTHON_LIBRARY})

SRSLTE_SET_PIC(srslte_oocran)
