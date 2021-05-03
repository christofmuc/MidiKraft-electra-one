#
#  Copyright (c) 2021 Christof Ruch. All rights reserved.
#
#  Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
#

cmake_minimum_required(VERSION 3.14)

project(MidiKraft-electra-one)

set(RESOURCE_FILES 
	"resources/instr.json"
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/BinaryResources.h
	COMMAND ${CMAKE_COMMAND} -P createResources.cmake	
	DEPENDS ${RESOURCE_FILES}
	COMMENT "Compiling binary resources"
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

# Define the sources for the static library
set(Sources		
	BinaryResources.h
	ElectraOne.cpp ElectraOne.h
	LICENSE.md	
	${RESOURCE_FILES}
)

set_source_files_properties(
	BinaryResources.h
	PROPERTIES GENERATED TRUE
)

# Setup library
add_library(midikraft-electra-one ${Sources})
target_include_directories(midikraft-electra-one PUBLIC ${CMAKE_CURRENT_LIST_DIR} PRIVATE ${JUCE_INCLUDES} ${boost_SOURCE_DIR} ${MANUALLY_RAPID_JSON} ${SQLITE_CPP_INCLUDE})
target_link_libraries(midikraft-electra-one juce-utils midikraft-base nlohmann_json::nlohmann_json nlohmann_json_schema_validator ${APPLE_BOOST})

# Pedantic about warnings
if (MSVC)
    # warning level 4 and all warnings as errors
    #target_compile_options(midikraft-database PRIVATE /W4 /WX)
else()
    # lots of warnings and all warnings as errors
    #target_compile_options(midikraft-librarian PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
