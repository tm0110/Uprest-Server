#include <stdio.h>
#include <stdlib.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

#include "Modules.hpp"

using namespace std;

namespace UprestModules
{

TimeModule::TimeModule(void)
{
	this->name = "time";
}

string TimeModule::Get(void)
{
	string ret = "Time?";

	auto t = time(NULL);
	auto lt = *localtime(&t);
	ostringstream ss;

	ss << "{ \"time\": \"" << put_time(&lt, "%H:%M:%S") << "\" }";
	ret = ss.str();

	return ret;
}

bool TimeModule::Post(string data)
{
	return false;
}

bool TimeModule::Delete(void)
{
	return false;
}

/* ... */

GuestbookModule::GuestbookModule(void)
{
	this->name = "guestbook";

	this->entries.push_back("This is a default guestbook entry!");
}

string GuestbookModule::Get(void)
{
	ostringstream ss;

	ss << "[\n";
	pthread_mutex_lock(&this->mutex);
	for (size_t i = 0; i < this->entries.size(); i++) {
		if (i > 0)
			ss << ",\n";
		ss << "{ \"entry\": \"" << this->entries[i] << "\"}";
	}
	pthread_mutex_unlock(&this->mutex);


	ss << "\n]";
	return ss.str();
}

bool GuestbookModule::Post(string data)
{
	bool ret = true;

	pthread_mutex_lock(&this->mutex);
	if (this->entries.size() < 30)
		this->entries.push_back(data);
	else
		ret = false;
	pthread_mutex_unlock(&this->mutex);

	return ret;
}


bool GuestbookModule::Delete(void)
{
	pthread_mutex_lock(&this->mutex);
	this->entries.clear();
	pthread_mutex_unlock(&this->mutex);

	return true;
}

} // namespace UprestModules
