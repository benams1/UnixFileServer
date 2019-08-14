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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>



#define PORT 0x0da2
#define IP_ADDR 0x7f000001
#define QUEUE_LEN 20

int main(void)
{
	int listenS = socket(AF_INET, SOCK_STREAM, 0);
	int socket_opt_val = 1;
	setsockopt(listenS, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &socket_opt_val, sizeof(int));
	if (listenS < 0)
	{
		perror("socket");
		return 1;
	}
	struct sockaddr_in s = {0};
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	s.sin_addr.s_addr = htonl(IP_ADDR);
	if (bind(listenS, (struct sockaddr*)&s, sizeof(s)) < 0)
	{
		perror("bind");
		return 1;
	}
 if (listen(listenS, QUEUE_LEN) < 0)
	{
		perror("listen");
		return 1;
	}
    while(1){

	struct sockaddr_in clientIn ={0};
	int clientInSize = sizeof clientIn;
	int newfd = accept(listenS, (struct sockaddr*)&clientIn, (socklen_t*)&clientInSize);
	if (newfd < 0)
	{
		perror("accept");
		return 1;
	}
	int op=0,fileFd,fileSize;
	if (recv(newfd, &op, sizeof(op), 0) < 1)
	{
		perror("Could not receive op ");
		exit(1);
	}
	int filenameLength = 0;
	char* filename =0;
	char path[512]={0};
	struct stat st;

	switch (op)
	{
	case 1://download single file
		if (recv(newfd, &filenameLength, sizeof(filenameLength), 0) < 1)
		{
			perror("Could not receive filename length");
			exit(1);
		}
		filename = (char*) malloc( filenameLength + 1);
		memset(filename, 0, filenameLength);
		if (recv(newfd, filename, filenameLength, 0) < 1)
		{
			perror("Could not receive filename");
			exit(1);
		}
		strcat(path,"files/");
		strcat(path,filename);
		if (stat(path, &st) < 0)
		{
			int err = -1;
			send(newfd, &err, sizeof(err), 0);
			//close(newfd);
			//close(listenS);
			//exit(-1);	
		}else
		{
			printf("%s\n", filename);
			if (send(newfd, &st.st_size, sizeof(st.st_size), 0) < 0)
			{
				perror("Could not send filesize to client");
				exit(1);
			}
			 fileFd = open(filename, O_RDONLY);
			if (fileFd)
			{
				void* file = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fileFd, 0);
				if (file)
				{
					int count = send(newfd, file, st.st_size, 0);
					if (count != st.st_size)
						printf("Error writing to socket");
					munmap(file, st.st_size);
				}
			}
		}
		break;
	case 2://download multiple files
		
		break;
	case 3://upload files
		if (recv(newfd, &filenameLength, sizeof(filenameLength), 0) < 1)
		{
			perror("Could not receive filename length");
			exit(1);
		}
		 filename = (char*) malloc(filenameLength + 1);
		memset(filename, 0, filenameLength);
		if (recv(newfd, filename, filenameLength, 0) < 1)
		{
			perror("Could not receive filename");
			exit(1);
		}
		strcat(path,"files/");
		strcat(path,filename);
		recv(newfd,&fileSize,sizeof(fileSize),0);
		//if file size less then zero
		if(fileSize<0){
			close(newfd);
			continue;
		}
		fileFd = open(path,O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		ftruncate(fileFd, fileSize);
		void* addr = mmap(0, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileFd, 0);
		char *data = malloc(sizeof(fileSize));
		int count = recv(newfd, addr, fileSize, MSG_WAITALL);
		if (count < 0)
		{
			printf("error receive: %s\n",strerror(errno));
		}
		munmap(addr, fileSize);
		close(fileFd);
		break;
	default:
		printf("error in op\n");
		break;
	}
	close(newfd);
    }
	close(listenS);
	return 0;
}