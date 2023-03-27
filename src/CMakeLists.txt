set(EXEC ${CMAKE_PROJECT_NAME})
file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.cpp)
set(SOURCES ${SOURCES})

add_executable(${EXEC}_run ${SOURCES})
add_library(${EXEC}_lib STATIC ${SOURCES})

set(LIBRARY_LINK_FLAGS -lglfw -lvulkan -ldl -lGL -lm)

target_link_libraries(${EXEC}_run ${LIBRARY_LINK_FLAGS})
target_link_libraries(${EXEC}_lib ${LIBRARY_LINK_FLAGS})