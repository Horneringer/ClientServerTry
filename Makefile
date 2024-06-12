all: build_server build_client

build_server:
	g++ -ggdb -O0  -o server server.cpp -levent

build_client:
	g++ -ggdb -O0  -o client client.cpp -levent
