/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
/* Leave this one at the bottom to keep cygwin happy */
#include <netinet/in.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(network);



int network_setup_on_port(int *fd, u16 port)
{
   	struct sockaddr_in s;
	int opts = 1;

	/* Bind onto a port, and wait */
	*fd = socket(AF_INET,SOCK_STREAM,0);
	
	s.sin_family = AF_INET;
	s.sin_port = htons(port);
	s.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(int));
	
	TRACE("Binding to address %s\n", inet_ntoa(s.sin_addr));
	
	if(bind(*fd,(struct sockaddr *)&s,sizeof(struct sockaddr_in)) < 0) {
/*		ERR("bind: errno=%d\n", errno);
		perror("Network_SetupOnPort:bind");*/
		return -1;
	}
	if(listen(*fd,0) < 0) {
		perror("Network_SetupOnPort:listen");
		return -1;
	}
	TRACE("Ready to accept a connection\n");

	return 0;
}

int network_accept(int *fd, int *client_fd)
{
	int status;
	status = accept(*fd, NULL, 0);
	TRACE("Accept returned %d\n", status);

	if(status == -1) {
		TRACE("Error condition is %d\n", errno);
		perror("accept:");
		return 0;
	}

	/* Leave the old bind fd open */
	*client_fd = status;

	/* Force telnet into  "echo off", "unbuffered" */
	send(*client_fd, "\xff\xfb\x01\xff\xfb\x03\xff\xfd\x0f3", 9, 0);
	return 1;
}



int network_check_accept(int *fd)
{
	int status;
	fd_set set;
	struct timeval tv;

	FD_ZERO(&set);
	FD_SET(*fd, &set);
	tv.tv_sec = tv.tv_usec = 0;

	if(!select((*fd)+1, &set, NULL, NULL, &tv))
		return 0;
	
	status = accept(*fd, NULL, 0);
	TRACE("Accept returned %d\n", status);

	if(status == -1) {
		TRACE("Error condition is %d\n", errno);
		perror("accept:");
		return 0;
	}

	/* Close the old (bind) fd, and setup the new (client) one */
	close(*fd);
	*fd = status;
	fcntl(*fd, F_SETFL, O_NONBLOCK);

	/* Force telnet into  "echo off", "unbuffered" */
	send(*fd, "\xff\xfb\x01\xff\xfb\x03\xff\xfd\x0f3", 9, 0);

	return 1;
}
