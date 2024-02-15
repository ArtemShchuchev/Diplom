cmake_minimum_required(VERSION 3.19)

set(OUTPUT_NAME Spider)
set(HEADERS ${PROJECT_SOURCE_DIR}/Types.h
			${PROJECT_SOURCE_DIR}/SecondFunk/SecondaryFunction.h
			${PROJECT_SOURCE_DIR}/HtmlClient/HtmlClient.h
			${PROJECT_SOURCE_DIR}/ConfigFile/ConfigFile.h
			${PROJECT_SOURCE_DIR}/Clientdb/Clientdb.h
			${PROJECT_SOURCE_DIR}/wordSearch/wordSearch.h
			${PROJECT_SOURCE_DIR}/ThreadPool/Thread_pool.h
			${PROJECT_SOURCE_DIR}/ThreadPool/Safe_queue.hpp
			${PROJECT_SOURCE_DIR}/UrlEncodeDecode/UrlEncodeDecode.h
)
set(SOURCES main.cpp
			${PROJECT_SOURCE_DIR}/SecondFunk/SecondaryFunction.cpp
			${PROJECT_SOURCE_DIR}/HtmlClient/HtmlClient.cpp
			${PROJECT_SOURCE_DIR}/ConfigFile/ConfigFile.cpp
			${PROJECT_SOURCE_DIR}/Clientdb/Clientdb.cpp
			${PROJECT_SOURCE_DIR}/wordSearch/wordSearch.cpp
			${PROJECT_SOURCE_DIR}/ThreadPool/Thread_pool.cpp
			${PROJECT_SOURCE_DIR}/UrlEncodeDecode/UrlEncodeDecode.cpp
)
# C/C++: доп. каталоги включаемых файлов (#include)
include_directories(${PROJECT_SOURCE_DIR}/SecondFunk
					${PROJECT_SOURCE_DIR}/HtmlClient
					${PROJECT_SOURCE_DIR}/ConfigFile
					${PROJECT_SOURCE_DIR}/Clientdb
					${PROJECT_SOURCE_DIR}/wordSearch
					${PROJECT_SOURCE_DIR}/ThreadPool
					${PROJECT_SOURCE_DIR}/UrlEncodeDecode
)

# Boost:
# Компоновщик: доп. каталоги библиотек
link_directories($ENV{BOOST_ROOT}/lib)

add_executable(${OUTPUT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${OUTPUT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(${OUTPUT_NAME} PRIVATE
										${Boost_LIBRARIES}
										pqxx
										OpenSSL::SSL)
