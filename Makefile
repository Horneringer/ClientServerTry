all: build_server build_client

build_server:
	g++ -ggdb -O0  -o server server.cpp

build_client:
	g++ -ggdb -O0  -o client client.cpp
