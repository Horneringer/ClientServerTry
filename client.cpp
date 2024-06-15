#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <sys/socket.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

/*КЛИЕНТ*/

//макрос обработки ошибок. Выводит сообщение об ошибке и завершает программу
#define handle_error(msg) \
 do                       \
 {                        \
  perror(msg);            \
  exit(EXIT_FAILURE);     \
 }                        \
 while (0)

const char* getCurrentDateTime()
{
	// Статическая переменная для хранения результата
	static char formattedDateTime[35];

	struct timespec currentTimeSpec;
	clock_gettime(CLOCK_REALTIME, &currentTimeSpec);

	// Преобразование времени в строку
	struct tm localTime;
	localtime_r(&currentTimeSpec.tv_sec, &localTime);
	strftime(formattedDateTime,
			 sizeof(formattedDateTime),
			 "%Y-%m-%d %H:%M:%S ",
			 &localTime);

	// Добавление миллисекунд к строке времени
	snprintf(formattedDateTime + 19,
			 sizeof(formattedDateTime) - 19,
			 ".%03ld",
			 currentTimeSpec.tv_nsec / 1000000);

	return formattedDateTime;
}

void client_task(const std::string& clientName,
				 const std::string& port,
				 const std::string& timeout)
{
	//текущая дата
	const char* curTime = getCurrentDateTime();

	//буфер для сообщений
	char Buffer[1024];

	size_t bufferSize = sizeof(Buffer);

	memset(Buffer, 0, sizeof(Buffer));

	//структура таймаута для сокета
	struct timeval tv;
	tv.tv_sec  = stoi(timeout);
	tv.tv_usec = 0;

	//структура для хранения адреса, к которому присоединяемся
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port	= htons(stoi(port));

	//подключаемся к адресу хоста
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	//создание сокета клиента
	int cfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (cfd == -1)
		handle_error("socket");

	//подключение к серверу
	if (connect(cfd, (sockaddr*)&addr, sizeof(addr)) == -1)
	{
		std::cerr << "Connect failed: " << strerror(errno) << "\n";
		handle_error("connect");
	}

	std::ostringstream oss;
	oss << "[" << curTime << "]"
		<< " " << clientName;
	std::string message = oss.str();

	//добавление в буфер текущего времени и даты, имени и таймаута
	std::strncat(Buffer, message.c_str(), message.size());

	//отправляем сообщение
	if (send(cfd, Buffer, bufferSize, MSG_NOSIGNAL) == -1)
		handle_error("send");

	// Имитация работы клиента
	// std::this_thread::sleep_for(std::chrono::seconds(std::stoi(timeout)));

	shutdown(cfd, SHUT_RDWR);
	close(cfd);
}

int main(int argc, char* argv[])
{
	//получение имени клиента, порта и периода подключения из консоли
	std::string clientName;
	std::string port;
	std::string timeout;

	std::cout << "Введите имя клиента"
			  << "\n";
	getline(std::cin, clientName);

	std::cout << "Введите порт"
			  << "\n";

	getline(std::cin, port);

	std::cout << "Введите период подключения"
			  << "\n";
	getline(std::cin, timeout);

	std::thread th(client_task, clientName + "2", port, timeout);
	std::this_thread::sleep_for(std::chrono::seconds(stoi(timeout) - 2));

	std::thread th2(client_task, clientName + "3", port, timeout);

	// Основной поток
	for (int i = 0; true; ++i)
	{
		client_task(clientName + "1", port, timeout);
		std::this_thread::sleep_for(std::chrono::seconds(stoi(timeout) - 4));
	}

	th.join();
	th2.join();

	return 0;
}