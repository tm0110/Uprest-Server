#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <string>
#include <vector>
#include <map>

using namespace std;

namespace Uprest
{

enum ExceptionType
{
	UPREST_ERROR_CREATING_SOCKET,
	UPREST_ERROR_BINDING,
	UPREST_ERROR_LISTENING,
	UPREST_ERROR_CREATING_THREAD
};

enum BindType
{
	UPREST_BIND_ANY,
	UPREST_BIND_LOCALHOST
};

enum RequestType
{
	UPREST_GET,
	UPREST_POST,
	UPREST_DELETE
};

enum ResponseType
{
	UPREST_OK,
	UPREST_BADREQUEST,
	UPREST_NOTFOUND
};

class Exception : public exception
{
public:
	ExceptionType exType;

	Exception(ExceptionType exType)
	{
		this->exType = exType;
	}
};

class Request
{
public:
	RequestType type;
	string      uri;
	string      data;

};

class Response
{
public:
	ResponseType type;
	string       data;
};

class Module
{
public:
	string name;

	virtual string Get(void) = 0;
	virtual bool   Post(string data) = 0;
	virtual bool   Delete(void) = 0;
};

class Server
{
public:
	 Server(BindType bindType, int port, string uriPrefix, int numThreads);
	~Server(void);

	void RegisterModule(Module *module);
	void Listen(void);
	void HandleConnections(void);
private:
        int                listenerSocket;
        struct sockaddr_in address;

	string uriPrefix;
	int    numThreads;

	atomic<int> running;

	vector<int> connections;

	pthread_t      *threads;
	pthread_mutex_t mutexConnections = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mutexCondition   = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t  condition        = PTHREAD_COND_INITIALIZER;

	map<size_t, Module *> hashModuleMap;

	Response EvaluateRequest(Request &req);
	Request  ParseRequest(const char *raw) const;
	string   StringifyResponse(Response &response) const;
	bool     ValidateRequest(const char *raw) const;
	string   SanitizeURI(string uri) const;
};

} // namespace Uprest

#endif
