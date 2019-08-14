#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 0x0da2
#define IP_ADDR 0x7f000001

int main(int argc, char* argv[])
{
	int op;
	if (argc == 1)
	{
		printf("Missing filename argument\n");
		exit (-1);
	}
	if(!strcmp(argv[1],"download")){
		if(argc == 3){
			op =1;
		}else{
			op=2;
		}
	}
	else if(!strcmp(argv[1],"upload")){
		op = 3;
	}else{
		printf("bad operation argument\n");
		exit (-1);
	}
	//connect to remote server
	int sock = socket(AF_INET, SOCK_STREAM, 0), nrecv;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
	struct sockaddr_in s = {0};
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	s.sin_addr.s_addr = htonl(IP_ADDR);
	struct stat st;
	if (connect(sock, (struct sockaddr*)&s, sizeof(s)) < 0)
	{
		perror("connect");
		return 1;
	}
	printf("Successfully connected.\n");
	//send op to server
	if (send(sock, (const void*) &op, sizeof (op),0) < 1)
	{
		perror("Could not send op to server");
		return 1;
	}
	//exit(0);
	int fd,length,fileSize;
	switch (op)
	{
	case 1:
		//send filename length to server
		length = strlen(argv[2]);
		if (send(sock, (const void*) &length, sizeof (length),0) < 1)
		{
			perror("Could not send file name length to server");
			return 1;
		}

		if (send(sock, (const void*) argv[2], length, 0) < 1)
		{
			perror("Could not send file name to server");
		}
		 fileSize = 0;
		recv(sock, &fileSize, sizeof(fileSize), 0);
		if (fileSize < 0)
		{
			perror("File not found");
			exit(1);
		}
		char fileName[256] = {0};
		strcat(fileName, argv[2]);
		strcat(fileName, ".copy");
		fd = open(fileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		//we didn't get to see this function in class, what it does essentially is to allocate and zero a file described by fd, with fileSize bytes.
		ftruncate(fd, fileSize);
		void* addr = mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		char *data = malloc(sizeof(fileSize));
		int count = recv(sock, addr, fileSize, MSG_WAITALL);
		if (count < 0)
		{
			printf("error receive: %s\n",strerror(errno));
		}
		munmap(addr, fileSize);
		break;
	case 2:
		break;
	case 3:
		length = strlen(argv[2]);
		if (send(sock, (const void*) &length, sizeof (length),0) < 1)
		{
			perror("Could not send file name length to server");
			return 1;
		}

		if (send(sock, (const void*) argv[2], length, 0) < 1)
		{
			perror("Could not send file name to server");
		}
		fileSize = 0;
		if(stat(argv[2],&st) < 0){
			int err =-1;
			send(sock,&err,sizeof(err),0);
			close(sock);
			exit(-1);
		}else{
			printf("%s\n",argv[2]);
			if (send(sock, &st.st_size, sizeof(st.st_size), 0) < 0)
			{
				perror("Could not send filesize to client");
				exit(1);
			}
			 fd = open(argv[2], O_RDONLY);
			if (fd)
			{
				void* file = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
				if (file)
				{
					send(fd, file, st.st_size, 0);
					munmap(file, st.st_size);
				}
			}
		}
		break;
	}
	
	close(fd);

}