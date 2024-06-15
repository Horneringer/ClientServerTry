all: build_server build_client

build_server:
	g++ -ggdb -O0  -o server server.cpp -lev

build_client:
	g++ -ggdb -O0  -o client client.cpp -lev
