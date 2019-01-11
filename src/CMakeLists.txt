cmake_minimum_required(VERSION 3.10)

set(BIN_NAME code_generator)

project(${BIN_NAME})
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

IF(APPLE)
  set(REFLEX_LIB "/usr/local/lib/libreflex.a")
  set(REFLEX_INCLUDE "/usr/local/include/reflex")
  set(REFLEX reflex)
ELSE(APPLE)
  set(REFLEX_LIB "/home/faculty/michal/classes/cis461/RE-flex/lib/libreflex.a")
  set(REFLEX_INCLUDE "/home/faculty/michal/classes/cis461/RE-flex/include")
  set(REFLEX "/home/faculty/michal/classes/cis461/RE-flex/bin/reflex")
  link_directories("/home/users/zayd/boost_1_68_0/stage/lib")
ENDIF(APPLE)

# Run re-Flex
add_custom_command(
    COMMAND  ${REFLEX} --bison-cc --bison-locations --header-file ${CMAKE_CURRENT_SOURCE_DIR}/quack.lxx
    OUTPUT   lex.yy.cpp lex.yy.h
    DEPENDS  quack.lxx quack.tab.hxx
)
# Run Bison
add_custom_command(
    COMMAND bison -d -v ${CMAKE_CURRENT_SOURCE_DIR}/quack.yxx --report-file=debug.output
    OUTPUT  quack.tab.cxx quack.tab.hxx location.hh position.hh stack.hh
    DEPENDS quack.yxx
)

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${REFLEX_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)

add_executable(${BIN_NAME}
               main.cpp
               quack_program.h
               quack_class.h quack_class.cpp
               quack_param.h quack_param.cpp
               quack_field.h
               lex.yy.h lex.yy.cpp quack.tab.cxx
               messages.h messages.cpp
               ASTNode.h ASTNode.cpp
               type_checker.h
               symbol_table.h
               initialized_list.h
               exceptions.h
               compiler_utils.h
               code_generator.h
               code_gen_utils.h)

target_link_libraries(${BIN_NAME} ${REFLEX_LIB})
