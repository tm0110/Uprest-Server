#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <string>
#include <sstream>
#include <cstring>
#include <unordered_map>

#include "Server.hpp"

using namespace std;

namespace Uprest
{

static void *ThreadEntry(void *ptr);

Server::Server(BindType bindType, int port, string uriPrefix, int numThreads)
{
	this->uriPrefix  = uriPrefix;
	this->numThreads = numThreads;

	assert(bindType == UPREST_BIND_LOCALHOST || bindType == UPREST_BIND_ANY);
	assert(numThreads >= 1 && numThreads <= 32);

	if ((this->listenerSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		throw Exception(UPREST_ERROR_CREATING_SOCKET);
	
	memset(static_cast<void *>(&this->address), 0, sizeof (this->address));

	address.sin_family = AF_INET;
	address.sin_port   = htons(port);

	if (bindType == UPREST_BIND_LOCALHOST)
		address.sin_addr.s_addr = inet_addr("127.0.0.1");
	else if (bindType == UPREST_BIND_ANY)
		address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (::bind(this->listenerSocket, reinterpret_cast<struct sockaddr *>(&this->address), sizeof (this->address)) < 0)
		throw Exception(UPREST_ERROR_BINDING);

	if (listen(this->listenerSocket, 4) < 0)
		throw Exception(UPREST_ERROR_LISTENING);

	this->running = 1;
	this->threads = new pthread_t[numThreads];
	for (int i = 0; i < numThreads; i++) {
		int check = pthread_create(&this->threads[i], NULL, ThreadEntry, this);

		if (check != 0)
			throw Exception(UPREST_ERROR_CREATING_THREAD);
	}
}

Server::~Server(void)
{
	this->running = 0;

	pthread_mutex_lock(&this->mutexCondition);
	pthread_cond_broadcast(&this->condition);
	pthread_mutex_unlock(&this->mutexCondition);

	for (int i = 0; i < this->numThreads; i++)
		pthread_join(this->threads[i], NULL);
	delete this->threads;

	close(this->listenerSocket);
}

void Server::RegisterModule(Module *module)
{
	hash<string> stringHasher;
	size_t       hash;

	hash = stringHasher(module->name);
	this->hashModuleMap[hash] = module;
}

void Server::Listen(void)
{
	socklen_t length = sizeof (this->address);

	errno = 0;

	while (1) {
		int sock;

		sock = accept(this->listenerSocket, reinterpret_cast<struct sockaddr *>(&this->address), &length);
		if (sock == -1) {
			if (errno == EINTR) 
				return;
			else
				continue;
		}

		pthread_mutex_lock(&this->mutexConnections);
		this->connections.push_back(sock);
		pthread_mutex_unlock(&this->mutexConnections);

		pthread_mutex_lock(&this->mutexCondition);
		pthread_cond_broadcast(&this->condition);
		pthread_mutex_unlock(&this->mutexCondition);
	}

	return;
}

void Server::HandleConnections(void)
{
	while (1) {
		char buf[8192] = {'\0'};
		int  sock;

		pthread_mutex_lock(&this->mutexCondition);
		pthread_cond_wait(&this->condition, &this->mutexCondition);
		pthread_mutex_unlock(&this->mutexCondition);

		if (running == 0)
			return;

		pthread_mutex_lock(&this->mutexConnections);
		if (this->connections.size() == 0) {
			pthread_mutex_unlock(&this->mutexConnections);
			continue;
		}

		sock = this->connections.front();
		this->connections.erase(this->connections.begin());
		pthread_mutex_unlock(&this->mutexConnections);

		read(sock, buf, sizeof (buf));

		if (ValidateRequest(buf) == false) {
			close(sock);
			continue;
		}

		Request  request;
		Response response;

		request     = ParseRequest(buf);
		request.uri = SanitizeURI(request.uri);

		if (request.uri.find(this->uriPrefix) != 0)
			response.type = UPREST_NOTFOUND;
		else
			response = EvaluateRequest(request);

		string strResponse = StringifyResponse(response);

		write(sock, strResponse.c_str(), strResponse.length());
		close(sock);
	}
}

Response Server::EvaluateRequest(Request &req)
{
	Response response;

	if (req.uri.find(this->uriPrefix) != 0) {
		response.type = UPREST_NOTFOUND;
		return response;
	} else
		response.type = UPREST_OK;

	response.data = "This is a default response!";

	string uriMinusPrefix = req.uri.substr(this->uriPrefix.length());
	uriMinusPrefix = SanitizeURI(uriMinusPrefix);

	if (uriMinusPrefix == "/meta/uprest/")
		response.data = "1";
	else if (uriMinusPrefix == "/meta/version/")
		response.data = "{ \"version\": \"1.0\" }";
	else if (uriMinusPrefix.find("/module/") == 0 && uriMinusPrefix.length() > 8) {
		Module      *module;
		string       moduleName;
		size_t       moduleHash;
		hash<string> strHasher;

		moduleName = uriMinusPrefix.substr(8);
		moduleName.pop_back(); // Remove trailing forward-slash

		moduleHash = strHasher(moduleName);
		module = this->hashModuleMap[moduleHash];
		if (module != NULL) {
			if (req.type == UPREST_GET) {
				response.data = module->Get();
			} else if (req.type == UPREST_POST) {
				if (module->Post(req.data) == true)
					response.data = "1";
				else
					response.type = UPREST_BADREQUEST;
			} else if (req.type == UPREST_DELETE) {
				if (module->Delete() == true)
					response.data = "1";
				else
					response.type = UPREST_BADREQUEST;
			}

		} else
			response.type = UPREST_NOTFOUND;
	}

	return response;
}

Request Server::ParseRequest(const char *raw) const
{
	Request request;

	string strRaw = raw;

	size_t posSecondLine = strRaw.find('\n');

	string methodURIProtocol = strRaw.substr(0, posSecondLine);

	size_t posURI = methodURIProtocol.find(' ');

	string method = methodURIProtocol.substr(0, posURI);

	string uriProtocol = methodURIProtocol.substr(posURI);

	uriProtocol = uriProtocol.substr(1);

	string uri = uriProtocol.substr(0, uriProtocol.find(' '));

	size_t posBody = strRaw.find("\n\r");

	if (posBody != string::npos)
		request.data = strRaw.substr(posBody + 3);

	if (method == "GET")
		request.type = UPREST_GET;
	else if (method == "POST")
		request.type = UPREST_POST;
	else if (method == "DELETE")
		request.type = UPREST_DELETE;

	request.uri = uri;

	return request;
}

string Server::StringifyResponse(Response &response) const
{
	string       str;
	stringstream ss;

	if (response.type == UPREST_BADREQUEST) {
		ss << "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\nContent-Length:0\n\n";
	} else if (response.type == UPREST_NOTFOUND) {
		ss << "HTTP/1.1 404 Not Found\nContent-Type: text/plain\nContent-Length:0\n\n";
	} else if (response.type == UPREST_OK) {
		size_t dataLength = response.data.length();

		ss << "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length:";

		ss << dataLength;
		ss << "\n\n" << response.data;
	}

	return ss.str();
}

bool Server::ValidateRequest(const char *raw) const
{
	size_t pos;
	string strReq = raw;

	if (strReq.length() == 0)
		return false;

	pos = strReq.find('\n');

	if (pos == string::npos)
		return false;

	string methodURIProtocol = strReq.substr(0, pos);

	pos = methodURIProtocol.find("HTTP/");

	if (pos == string::npos)
		return false;

	return true;
}

string Server::SanitizeURI(string uri) const
{
	size_t pos;

	if (uri.length() == 0)
		return uri;
	if (uri.front() != '/')
		uri.insert(0, "/");
	if (uri.back() != '/')
		uri.insert(uri.length(), "/");

	while( (pos = uri.find("//")) != string::npos)
		uri.erase(pos);

	return uri;
}

static void *ThreadEntry(void *ptr)
{
	Server *server;

	server = static_cast<Server *>(ptr);
	server->HandleConnections();

	return NULL;
}

} // namespace Uprest
