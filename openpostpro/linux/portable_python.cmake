# Run by CPack after the package is made (CPACK_POST_BUILD_SCRIPTS): see portable-python.sh
foreach(package ${CPACK_PACKAGE_FILES})
	execute_process(COMMAND sh ${CPACK_OPP_LINUX_DIR}/portable-python.sh ${package} ${CPACK_OPP_LINUX_DIR}
		RESULT_VARIABLE result)
	if(NOT result EQUAL 0)
		message(WARNING "portable-python.sh failed on ${package}, the package stays tied to the Python of this system")
	endif()
endforeach()
