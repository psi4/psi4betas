set(headers_list "")
# List of headers
list(APPEND headers_list MOInfo.h Params.h ccwave.h Local.h )

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
list(APPEND sources_list local.cc FT2.cc status.cc Fmi.cc cc2_fmiT2.cc form_df_ints.cc WmnijT2.cc analyze.cc rotate.cc cc2_Wmnij.cc cache.cc cc3_Wmnij.cc FaetT2.cc cc2_WmbijT2.cc spinad_amps.cc tsave.cc priority.cc BT2_AO.cc cc2_t2.cc get_params.cc AO_contribute.cc Wmnij.cc converged.cc WmbejT2.cc mp2_energy.cc ccenergy.cc sort_amps.cc diis_ROHF.cc fock_build.cc cc3.cc FT2_cc2.cc cc2_WabeiT2.cc diis.cc Wmbej.cc cc3_Wmnie.cc cc3_Wmbij.cc diis_RHF.cc dijabT2.cc halftrans.cc init_amps.cc CT2.cc cc2_faeT2.cc cc2_WabijT2.cc t2.cc ZT2.cc get_moinfo.cc update.cc Fme.cc d1diag.cc amp_write.cc Fae.cc Z.cc FmitT2.cc ET2.cc energy.cc lmp2.cc BT2.cc diis_UHF.cc tau.cc cc2_Wmbij.cc new_d1diag.cc cc3_Wabei.cc cc3_Wamef.cc t1.cc pair_energies.cc taut.cc denom.cc DT2.cc diagnostic.cc cc2_Wabei.cc d2diag.cc )

# If you want to remove some sources specify them explictly here
if(DEVELOPMENT_CODE)
   list(REMOVE_ITEM sources_list "")
else()
   list(REMOVE_ITEM sources_list "")
endif()


# Build static library
add_library(ccenergy STATIC ${sources_list})
# Specify dependencies for the library (if any)
add_dependencies(ccenergy mints)
set_property(GLOBAL APPEND PROPERTY PSILIB ccenergy)
if(BUILD_CUSTOM_BOOST)
   add_dependencies(ccenergy custom_boost)
endif()

# Sets install directory for all the headers in the list
install_list_FILES("${headers_list}" include/psi4/bin/ccenergy)
