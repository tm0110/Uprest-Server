#ifndef MODULES_HPP
#define MODULES_HPP

#include <vector>
#include <pthread.h>

#include "Server.hpp"

using namespace std;
using namespace Uprest;

namespace UprestModules
{

class TimeModule : public Module
{
public:
	TimeModule(void);

	string Get(void);
	bool   Post(string data);
	bool   Delete(void);
};

class GuestbookModule : public Module
{
public:
	GuestbookModule(void);

	string Get(void);
	bool   Post(string data);
	bool   Delete(void);

private:
	vector<string>  entries;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
};


} // namespace UprestModules

#endif
