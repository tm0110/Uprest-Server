# Uprest-Server

RESTful backend server written in C++.

Fast, extensible, multi-threaded. Compiles and runs on Linux, FreeBSD, and macOS. 

Easily set up to run behind a reverse proxy (see below).

# Requirements

CMake

# Building & Running

		cmake .
		make
		./uprest-server

# Configuration & Reverse Proxy (nginx)

Default port is 4444, and the default URI prefix (base URI) is /uprest/.

Both can be changed in main.c, line 38:

		server = new Server(UPREST_BIND_LOCALHOST, UPREST_PORT, "/uprest/", 4);

For running Uprest-Server behind an nginx reverse proxy, add the following to your nginx configuration:

		location /uprest {
			proxy_pass http://127.0.0.1:4444;
		}

Special note on CORS (Cross-Origin Resource Sharing): it is recommended to run Uprest-Server and any
front-end from behind the same HTTP server, to avoid any issues with CORS.

Otherwise, consider this partial workaround (in nginx configuration server block):

		add_header Access-Control-Allow-Origin *;
