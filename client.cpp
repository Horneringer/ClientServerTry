#include <cstring>
#include <iostream>
#include <netinet/in.h>
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
	static char
					formattedDateTime[35]; // Статическая переменная для хранения результата
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

	//буфер для соотщений
	char   Buffer[1024];
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
		handle_error("connect");

	//добавление в буфер текущего времени и даты
	std::strncat(Buffer, curTime, strlen(curTime));
	//добавление в буфер введённого имя клиента
	std::strncat(Buffer, clientName.c_str(), sizeof(clientName));
	//добавление в буфер периода подключения
	std::strncat(Buffer, timeout.c_str(), sizeof(timeout));

	//отправляем сообщение
	if (send(cfd, Buffer, bufferSize, MSG_NOSIGNAL) == -1)
		handle_error("send");

	shutdown(cfd, SHUT_RDWR);
	close(cfd);
}

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cerr << "Usage: " << argv[0]
				  << " < clientName > < port > < timeout>\n";
		return 1;
	}

	std::string clientName = argv[1];
	std::string port	   = argv[2];
	std::string timeout	   = argv[3];

	std::vector<std::thread> threads;

	//запуск клиентского кода в 10 потоках
	for (int i = 0; i < 10; ++i)
	{
		clientName += std::to_string(i);
		threads.emplace_back(client_task, clientName, port, timeout);

		//задержка в 5 секунд между запусками клиентов
		std::this_thread::sleep_for(std::chrono::seconds(stoi(timeout)));
		clientName.erase(clientName.end() - 1);
	}

	for (auto& thread : threads)
		thread.join();

	return 0;
}