#include <iostream>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <time.h>

/*КЛИЕНТ*/

//макрос обработки ошибок. Выводит сообщение об ошибке и завершает программу
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

const char* getCurrentDateTime() 
{
    static char formattedDateTime[35]; // Статическая переменная для хранения результата
    struct timespec currentTimeSpec;
    clock_gettime(CLOCK_REALTIME, &currentTimeSpec);

    // Преобразование времени в строку
    struct tm localTime;
    localtime_r(&currentTimeSpec.tv_sec, &localTime);
    strftime(formattedDateTime, sizeof(formattedDateTime), "%Y-%m-%d %H:%M:%S", &localTime);

    // Добавление миллисекунд к строке времени
    snprintf(formattedDateTime + 19, sizeof(formattedDateTime) - 19, ".%03ld", currentTimeSpec.tv_nsec / 1000000);

    return formattedDateTime;
}

int main()
{
    const char* curTime = getCurrentDateTime();
    char Buffer[1024];
    memset(Buffer, 0, sizeof(Buffer));
    //получение имени клиента, порта и периода подключения из консоли
    std::string clientName;
    std::string port;
    std::string timeout;
    
    std::cout << "Введите имя клиента" << "\n";
    getline(std::cin, clientName);

    std::cout << "Введите порт" << "\n";

    getline(std::cin, port);

    std::cout << "Введите период подключения" << "\n";
    getline(std::cin, timeout);

    //структура таймаута для сокета
    struct timeval tv;
    tv.tv_sec = stoi(timeout);
    tv.tv_usec = 0;

    //структура для хранения адреса, к которому присоединяемся
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(stoi(port));

    //подключаемся к адресу хоста
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    //создание сокета клиента   
    int cfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (cfd == -1)
        handle_error("socket");

    //установка таймаута для клиентского сокета, 
    //т.е. сколько он будет ждать ответа от сервера перед повтором попытки
    if (setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        handle_error("setsockopt");
    if (setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        handle_error("setsockopt");

    //подключение к серверу
    if(connect(cfd,(sockaddr *)&addr, sizeof(addr)) == -1)
        handle_error("connect");

    size_t bufferSize = sizeof(Buffer);


    //добавление в буфер текущего времени и даты
    std::strncat(Buffer, curTime, strlen(curTime));
    //добавление в буфер введённого имя клиента
    std::strncat(Buffer, clientName.c_str(), sizeof(clientName));
    //добавление в буфер периода подключения
    std::strncat(Buffer, timeout.c_str(), sizeof(timeout));

    //отправляем сообщение 
    if(send(cfd, Buffer, bufferSize, MSG_NOSIGNAL) == -1)
        handle_error("send");

    size_t recvSize = recv(cfd, Buffer, strlen(Buffer), MSG_NOSIGNAL);

    if (recvSize > 0)
        std::cout << "Data = " << Buffer << "\n";
    else if (recvSize == 0)
        std::cout << "Server closed";
    else
       handle_error("recv"); 
     

    shutdown(cfd, SHUT_RDWR);
    close(cfd);
    
    return 0;
}