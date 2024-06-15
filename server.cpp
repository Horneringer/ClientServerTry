#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
//для libev
#include <ev.h>
//для структур IPv4 и IPv6
#include <mutex>
#include <netinet/in.h>

/*СЕРВЕР*/

//глобальный мьютекс
std::mutex mtx;

//макрос обработки ошибок. Выводит сообщение об ошибке и завершает программу
#define handle_error(msg) \
 do                       \
 {                        \
  perror(msg);            \
  exit(EXIT_FAILURE);     \
 }                        \
 while (0)

//коллбэк на подключение
/*
цикл событий
вотчер
возвращаемое событие
*/
void read_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
	char Buffer[1024];

	memset(&Buffer, 0, sizeof(Buffer));

	//чтение данных из сокета
	size_t recvSize = recv(watcher->fd, Buffer, 1024, MSG_NOSIGNAL);

	if (recvSize < 0)
		handle_error("recv");

	//если соединение закрылось
	else if (recvSize == 0)
	{
		//остановка вотчера
		ev_io_stop(loop, watcher);

		//освобождение памяти под вотчер
		free(watcher);
		return;
	}

	else
	{
		std::lock_guard<std::mutex> lock(mtx);

		std::ofstream logFile;
		//открытие файла на запись и дозапись после каждого входящего соединения
		logFile.open("log.txt", std::ios::out | std::ios::app);

		if (!logFile.is_open())
			return;
		logFile << Buffer << "\n";

		logFile.close();
	}
}

void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents)
{
	//приём соединения
	int cfd = accept(watcher->fd, nullptr, nullptr);
	if (cfd == -1)
		handle_error("accept");

	//вотчер на чтение создаётся динамически, чтобы при выходе из коллбэка он
	//сохранился в памяти
	struct ev_io* w_client = (struct ev_io*)malloc(sizeof(struct ev_io));

	//инициализация вотчера на чтение
	//отслеживается событие на новом дескрипторе клиента
	ev_io_init(w_client, read_cb, cfd, EV_READ);

	ev_io_start(loop, w_client);
}

int main()
{
	//дескриптор сокета сервера
	int sfd;

	//получение порта из консоли
	std::string port;

	std::cout << "Введите порт"
			  << "\n";

	getline(std::cin, port);

	//создание главного цикла событий
	struct ev_loop* loop = ev_default_loop(0);

	//структура для хранения адреса серверного сокета
	struct sockaddr_in addr;

	//зануляем структуру адреса сокета
	memset(&addr, 0, sizeof(addr));

	//семейство доменов
	addr.sin_family = AF_INET;

	// htons используются для преобразования числовых значений из порядка байтов
	// хоста(little-endian) в порядок байтов сети(big-endian)
	addr.sin_port = htons(stoi(port));

	// ip адрес; INADDR_ANY - все доступные адреса; INADDR_LOOPBACK - ip хоста
	// 127.0.0.1 htonl используется для приведения ip адреса к сетевому виду
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0

	//создание сокета
	sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int optval = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))
		== -1)
		handle_error("setsockopt");

	//проверка, что он создался успешно
	if (sfd == -1)
		handle_error("socket");

	//привязка сокета к определённому адресу и порту

	//принимает:
	//  дескриптор серверного сокета,
	//  указатель на структуру адреса сокета,
	//  размер структуры адреса
	if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		handle_error("bind");

	//перевода сокета в режим пассивного ожидания входящих соединений
	//можно использовать константу SOMAXCON, которая хранит максимально
	//возможную длину очереди подключений для текущей ОС(остальным будет
	//отказано в подключении)
	if (listen(sfd, SOMAXCONN) == -1)
		handle_error("listen");

	//создание "вотчера", котрый следит за определёнными событиями и вызывает
	//связанык коллбэки

	//вотчер для отслеживания нового соединения
	struct ev_io w_accept;

	//инициализация вотчера
	// sfd - отслеживаемый дескриптор
	// EV_READ - отслеживаемое событие

	ev_io_init(&w_accept, accept_cb, sfd, EV_READ);

	//запуск вотчера
	ev_io_start(loop, &w_accept);

	//запуск цикла событий
	/*
	0: Цикл событий будет работать в блокирующем режиме до тех пор, пока есть
	активные события.
	EVRUN_ONCE: Цикл событий обработает одно событие и затем
	завершится.
	EVRUN_NOWAIT: Цикл событий немедленно завершится, если нет доступных
	событий для обработки.
  */

	ev_loop(loop, 0);

	return 0;
}