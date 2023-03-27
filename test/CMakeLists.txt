set(EXEC ${CMAKE_PROJECT_NAME}_test)
file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)
set(SOURCES ${TEST_SOURCES})

add_executable(${EXEC} ${TEST_SOURCES})
add_test(NAME ${EXEC} COMMAND ${EXEC})

set(LIBRARY_LINK_FLAGS -lglfw -lvulkan -ldl -lGL -lm)

target_link_libraries(${EXEC} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest ${LIBRARY_LINK_FLAGS})
