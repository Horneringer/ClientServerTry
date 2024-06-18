#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>

/*Клиент-сервер c использованием библиотеки boost::asio*/

//импорт пространства имён
using boost::asio::ip::tcp;

//класс, отвечающий за одно соединение клиента с сервером
class Session
{
private:

	//сокет
	tcp::socket sfd;

	enum
	{
		max_length = 1024
	};

	//буфер для работы с данными
	char data[max_length];

public:

	//конструктор принимает сервис ввода-вывода(аналог база событий в libevent)
	Session(boost::asio::io_service& io_service)
		: sfd(io_service)
	{}

	~Session() {}

	//функция возвращающая сокет
	tcp::socket& socket()
	{
		return sfd;
	}

	//чтение из соединения
	void start()
	{
		//асинхронное чтение
		// buffer создаётся на основе массива char
		//коллбэк, срабатывающий после чтения

		//при вызове bind передается указатель на текущий объект (this)
		//и аргументы для обработчика, такие как ошибка и количество переданных
		//байтов
		sfd.async_read_some(
		  boost::asio::buffer(data, max_length),
		  boost::bind(&Session::handle_read,
					  this,
					  boost::asio::placeholders::error,
					  boost::asio::placeholders::bytes_transferred));
	}

	//коллбэк на чтение
	void handle_read(const boost::system::error_code& error,
					 size_t							  bytes_transferred)
	{
		//если ошибка
		if (error)
			//удаляем сессию
			delete this;

		//если всё нормально, записываем в сокет столько байт, сколько пришло и
		//связываем коллбэк на запись с методом класса
		boost::asio::async_write(sfd,
								 boost::asio::buffer(data, bytes_transferred),
								 boost::bind(&Session::handle_write,
											 this,
											 boost::asio::placeholders::error));
	}

	void handle_write(const boost::system::error_code& error)
	{
		if (error)
			delete this;

		//если ошибок нет
		//вызов асинхронного чтения в буфер и привязывание коллбэка
		sfd.async_read_some(
		  boost::asio::buffer(data, max_length),
		  boost::bind(&Session::handle_read,
					  this,
					  boost::asio::placeholders::error,
					  boost::asio::placeholders::bytes_transferred));
	}
};

//класс сервера, обрабатывающий все соединения
class Server
{
private:

	//ссылка на сервис ввода-вывода
	boost::asio::io_service& io_service_;

	//объект для принятия входящих соединений
	tcp::acceptor acceptor_;

public:

	Server(boost::asio::io_service& io_service, short port)
		: io_service_(io_service),
		  acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
	{
		//создание новой сессии
		//принимает сервис ввода-вывода
		Session* new_session = new Session(io_service_);

		//принимаем соединение, вызываем соответствующий коллбэк, связанный с
		//методом класса, передавая указатель на сессию и ошибку
		acceptor_.async_accept(new_session->socket(),
							   boost::bind(&Server::handle_accept,
										   this,
										   new_session,
										   boost::asio::placeholders::error));
	}

	~Server() {}

	void handle_accept(Session*							new_session,
					   const boost::system::error_code& error)
	{
		if (error)
			delete new_session;

		//если ошибки нет, запускаем сессию
		//указатели на неё не храним, работает сама по себе и при ошибке	
		//чтения/записи может себя удалить
		new_session->start();

		//создаём новую сессию
		new_session = new Session(io_service_);

		//снова запускаем асинхронный приём соединений
		acceptor_.async_accept(new_session->socket(),
							   boost::bind(&Server::handle_accept,
										   this,
										   new_session,
										   boost::asio::placeholders::error));
	}
};

int main(int argc, char* argv[])
{
	boost::asio::io_service io_service;

	//объект сервера
	//указываем порт как параметр командной строки
	Server server(io_service, atoi(argv[1]));

	//запуск сервиса ввода-вывода
	io_service.run();

	return 0;
}