#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <fstream>

//для структур IPv4 и IPv6
#include <netinet/in.h>

/*СЕРВЕР*/

//макрос обработки ошибок. Выводит сообщение об ошибке и завершает программу
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main()
{
    //получение порта из консоли
    std::string port;
    //дескриптор сокета сервера
    int sfd;
    std::cout << "Введите порт" << "\n";

    getline(std::cin, port);
    
    //структура для хранения адреса серверного сокета
    struct sockaddr_in addr;

    //семейство доменов 
    addr.sin_family = AF_INET;
    //htons используются для преобразования числовых значений из порядка байтов хоста(little-endian) в порядок байтов сети(big-endian)
    addr.sin_port = htons(stoi(port));
    //ip адрес; INADDR_ANY - все доступные адреса; INADDR_LOOPBACK - ip хоста 127.0.0.1
    //htonl используется для приведения ip адреса к сетевому виду 
    addr.sin_addr.s_addr = htonl(INADDR_ANY); //0.0.0.0

    //структура для хранения адреса клиентского сокета
    struct sockaddr_in peer_addr;
    //размер структуры адреса клиентского сокета
    socklen_t peer_addr_size = sizeof(peer_addr);

    //создание сокета    
    sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //проверка, что он создался успешно
    if (sfd == -1)
        handle_error("socket"); 

    //разрешить повторное использование адреса и порта
    int optval = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    //привязка сокета к определённому адресу и порту

    //принимает: 
    //  дескриптор серверного сокета, 
    //  указатель на структуру адреса сокета, 
    //  размер структуры адреса
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        handle_error("bind");

    //перевода сокета в режим пассивного ожидания входящих соединений
    //можно использовать константу SOMAXCON, которая хранит максимально возможную длину очереди подключений для текущей ОС(остальным будет отказано в подключении) 
    if (listen(sfd, SOMAXCONN) == -1)
        handle_error("listen");

        //буфер для чтнеи/записи данных
        char Buffer[1024];

        std::ofstream logFile;

    while(1)
    {
        //принятия входящих соединений на сервере; блокируется и ожидает, пока клиент не попытается установить соединение с сервером
        //принимает: 
        //  серверный дескриптор
        //  указатель на структуру адреса клиентского сокета
        //  указатель на размер структуры адреса клиентского сокета
        //возвращает:
        //  новый дескриптор для общения с клиентом; создаётся в неблокируемом режиме
        int cfd = accept4(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size, SOCK_NONBLOCK);
        if (cfd == -1)
            handle_error("accept4");

        //сервер засыпает на 10 секунд для проверки работы таймаута
        sleep(10);
        
        //обнуление буфера
        memset(Buffer, 0, sizeof(Buffer));

        //для сокета  в неблокирующем режиме осуществляется проверка в цикле
        //если данные ещё не пришли(recv == -1)
        ssize_t numBytes;
        while ((numBytes = recv(cfd, Buffer, sizeof(Buffer) - 1, MSG_NOSIGNAL)) == -1)
        {
             //и errno содержит EAGAIN
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                //то спим 20 секунд, затем повторяем итерацию
                usleep(200000);
                continue;
            }
            else
                //иначе обработчик ошибок закрывает соединение 
                handle_error("recv");
        }
             
        // shutdown(cfd, SHUT_RDWR);
        // close(cfd);

        //открытие файла на запись и дозапись после каждого входящего соединения
        logFile.open("log.txt", std::ios::out | std::ios::app);
        if (!logFile.is_open())
            return 1;

        logFile << Buffer << "\n";

        logFile.close();
    }
    return 0;    
}