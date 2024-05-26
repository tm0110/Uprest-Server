#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <string>
#include <cstring>

#include "Server.hpp"
#include "Modules.hpp"

#define UPREST_PORT 4444

using namespace Uprest;
using namespace UprestModules;

static void on_interrupt_signal(int signal);

int main(int argc, char **argv)
{
	struct sigaction action;

	Server          *server;
	TimeModule       timeModule;
	GuestbookModule  guestbookModule;

	memset(&action, 0, sizeof (struct sigaction));

	action.sa_handler = on_interrupt_signal;
	action.sa_flags   = 0;

	sigemptyset(&action.sa_mask);
	sigaction(SIGINT, &action, NULL);

	printf("Starting Uprest Server (port %d)\n", UPREST_PORT);

	try {
		server = new Server(UPREST_BIND_LOCALHOST, UPREST_PORT, "/uprest/", 4);
	} catch (Exception e) {
		switch (e.exType) {
		case UPREST_ERROR_CREATING_SOCKET:
			printf("Exception caught: could not create socket\n");
			break;
		case UPREST_ERROR_BINDING:
			printf("Exception caught: could not bind\n");
			break;
		case UPREST_ERROR_LISTENING:
			printf("Exception caught: could not listen\n");
			break;
		case UPREST_ERROR_CREATING_THREAD:
			printf("Exception caught: error while creating thread\n");
			break;
		}

		return EXIT_FAILURE;
	}

	server->RegisterModule(&timeModule);
	server->RegisterModule(&guestbookModule);
	server->Listen();

	delete server;

	return EXIT_SUCCESS;
}

static void on_interrupt_signal(int signal)
{
	printf("\nCaught interrupt signal, gracefully shutting down...\n");
}
