set(headers_list "")
# List of headers
list(APPEND headers_list sapt2p3.h sapt.h sapt2.h sapt0.h sapt2p.h )

# If you want to remove some headers specify them explictly here
if(DEVELOPMENT_CODE)
   list(REMOVE_ITEM headers_list "")
else()
   list(REMOVE_ITEM headers_list "")
endif()
# Sort alphabetically
list(SORT headers_list)

set(sources_list "")
# List of sources
list(APPEND sources_list exch-ind-disp30.cc exch12.cc sapt2.cc sapt.cc sapt0.cc exch-disp30.cc exch-disp20.cc amplitudes.cc utils.cc disp20.cc ind30.cc disp22sdq.cc ind-disp30.cc sapt2p.cc elst13.cc disp30.cc disp22t.cc ind20.cc elst10.cc exch-ind30.cc exch11.cc disp21.cc disp2ccd.cc exch10.cc exch-ind20.cc elst12.cc sapt2p3.cc ind22.cc )

# If you want to remove some sources specify them explictly here
if(DEVELOPMENT_CODE)
   list(REMOVE_ITEM sources_list "")
else()
   list(REMOVE_ITEM sources_list "")
endif()


# Build static library
add_library(sapt_solver STATIC ${sources_list})
# Specify dependencies for the library (if any)
add_dependencies(sapt_solver mints)
set_property(GLOBAL APPEND PROPERTY LIBLIST sapt_solver)
if(BUILD_CUSTOM_BOOST)
   add_dependencies(sapt_solver custom_boost)
endif()

# Sets install directory for all the headers in the list
install_list_FILES("${headers_list}" include/psi4/lib/libsapt_solver)
