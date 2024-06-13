//для буфера
#include <event2/buffer.h>
//для структуры, отслеживающей события
#include <event2/bufferevent.h>
//для слушателя со стороны сервера
#include <event2/listener.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

//для структур IPv4 и IPv6
#include <netinet/in.h>

/*СЕРВЕР*/

//макрос обработки ошибок. Выводит сообщение об ошибке и завершает программу
#define handle_error(msg) \
 do                       \
 {                        \
  perror(msg);            \
  exit(EXIT_FAILURE);     \
 }                        \
 while (0)

//коллбэк на чтение
void read_cb(struct bufferevent* bev, void* arg)
{
	//достаём буферы входа/выхода из bufferevent
	struct evbuffer* input	= bufferevent_get_input(bev);
	struct evbuffer* output = bufferevent_get_output(bev);

	//получение текущей длины буфера
	size_t lenght = evbuffer_get_length(input);

	//выделение памяти для данных
	char* data = (char*)malloc(lenght + 1);
	if (!data)
	{
		perror("malloc");
		return;
	}

	//копирование данных из буфера в выделенный массив
	evbuffer_copyout(input, data, lenght);

	data[lenght] = '\0';

	std::ofstream logFile;

	//открытие файла на запись и дозапись после каждого входящего соединения
	logFile.open("log.txt", std::ios::out | std::ios::app);
	if (!logFile.is_open())
	{
		perror("Error open!");
		free(data);
		return;
	}

	logFile << data << "\n";

	logFile.close();

	std::cout << "Data: " << data << "\n";

	//перекидываем из одного буфера в другой
	evbuffer_add_buffer(output, input);

	//освобождение памяти
	free(data);
}

//коллбэк на запись
void event_cb(struct bufferevent* bev, short events, void* arg)
{
	//если произошла ошибка в событии буфера
	if (events & BEV_EVENT_ERROR)
	{
		std::perror("Error");
		//освобождение буфера событий
		bufferevent_free(bev);
	}

	//если данные передоваемые или принимаемые через событие закончились
	if (events & BEV_EVENT_EOF)
		bufferevent_free(bev);
}

/*
коллбэк на приём соединения

слушатель
серверный дескриптор
адрес серверного сокета
размер адреса
доп аргумент
*/
void accept_conn_cb(struct evconnlistener* listener,
					evutil_socket_t		   sfd,
					sockaddr*			   addr,
					int					   sockleln,
					void*				   arg)
{
	//указатель на базу, связанную со слушателем
	struct event_base* base = evconnlistener_get_base(listener);

	//указатель на буфер событий
	// флаг закрытия bufferevent после его освобождения
	struct bufferevent* bev =
	  bufferevent_socket_new(base, sfd, BEV_OPT_CLOSE_ON_FREE);

	//установка коллбэков на буфер событий
	bufferevent_setcb(bev, read_cb, nullptr, event_cb, nullptr);

	//регистрация событий для bufferevent
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

//коллбэк на случай ошибки
void accept_error_cb(struct evconnlistener* listener, void* arg)
{
	//указатель на базу, связанную со слушателем
	struct event_base* base = evconnlistener_get_base(listener);

	//код ошибки сокета
	int err = EVUTIL_SOCKET_ERROR();

	fprintf(stderr, "Socket error: %s\n", evutil_socket_error_to_string(err));

	//завершения работы базы событий
	event_base_loopexit(base, nullptr);
}

int main()
{
	//создание базы событий
	struct event_base* base = event_base_new();

	//дескриптор сокета сервера
	int sfd;

	//получение порта из консоли
	std::string port;
	std::cout << "Введите порт"
			  << "\n";

	getline(std::cin, port);

	//структура для хранения адреса серверного сокета
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	//семейство доменов
	addr.sin_family = AF_INET;

	// htons используются для преобразования числовых значений из порядка байтов
	// хоста(little-endian) в порядок байтов сети(big-endian)
	addr.sin_port = htons(stoi(port));

	// ip адрес; INADDR_ANY - все доступные адреса; INADDR_LOOPBACK - ip хоста
	// 127.0.0.1 htonl используется для приведения ip адреса к сетевому виду
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0

	//структура для хранения адреса клиентского сокета
	struct sockaddr_in peer_addr;

	//размер структуры адреса клиентского сокета
	socklen_t peer_addr_size = sizeof(peer_addr);

	//создание объекта слушателя

	/*
	база событий
	коллбэк, срабатывающий на новое подключение
	доп аргумент, передаваемый в коллбэк
	флаги:
	LEV_OPT_CLOSE_ON_FREE - закрывает сокет после особождении связанного события
	LEV_OPT_REUSEABLE - сокет может быть повторно использован после его закрытия
	размер очереди подключений
	структура адреса серверного сокета
	размер структуры адреса
	*/
	struct evconnlistener* listener =
	  evconnlistener_new_bind(base,
							  accept_conn_cb,
							  nullptr,
							  LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
							  -1,
							  (sockaddr*)&addr,
							  sizeof(addr));

	//установка коллбэка для обработки ошибок
	evconnlistener_set_error_cb(listener, accept_error_cb);

	//запуск цикла событий
	event_base_dispatch(base);

	//освобождение ресурсов
	evconnlistener_free(listener);
	event_base_free(base);

	return 0;
}
