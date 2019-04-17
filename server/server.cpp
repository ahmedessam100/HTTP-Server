#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>


using namespace std;


#define MAXPENDING 5
#define RCVBUFSIZE 1000000


void die(char *s){
     perror(s); /// prints a descriptive error message to stderr. First the string str is printed, followed by a colon then a space.
     exit(1);
}
void write_bytes_to_file(char *bytes,string filename,int read_size){
  FILE *file = fopen(filename.c_str(),"w");
  fwrite(bytes,1,read_size, file);
}
bool send_file(string filename,int clntSocket){
   FILE *file = fopen(filename.c_str(),"r");
   if(file == NULL){
       return false;
   }
   fseek(file, 0, SEEK_END);
   int size = ftell(file);
   fseek(file, 0, SEEK_SET);
   printf("file size is %d\n",size);

   char *buffer = new char[3000001];
   int packet_index = 1,stat;

   printf("Sending response\n");
   do{
     stat = send(clntSocket, "HTTP/1.0 200 OK\r\n", strlen("HTTP/1.0 200 OK\r\n"),0);
   }while(stat <0);

    printf("Sending file size\n");
    do{
      stat = send(clntSocket, (void *)&size, sizeof(int),0);
    }while(stat<0);

    printf("Sending file\n");
    int cur_size = 0;
    for(int i=0;i<size;i++){
      buffer[cur_size++] = getc(file);
      if(cur_size == 32000 || i+1 == size){
         do{
           stat = send(clntSocket, buffer, cur_size,0);
         }while (stat < 0);
         cur_size = 0;
         int z;
         while(recv(clntSocket,(void*)&z ,sizeof(int), 0)<0);
      }
   }

   return true;
}

vector<string> parse(char *msg)
{
    vector<string> parsed;
    string tmp = "";
    for(int i = 0;i <strlen(msg);i++)
    {
        char c= msg[i];
        if(c == ' '){
          if(tmp.length())
            parsed.push_back(tmp);
            tmp = "";
        }
        else
            tmp += c;
    }
    if(tmp.length())
        parsed.push_back(tmp);
    return parsed;
}

void HandleClient(int clntSocket){
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    memset(echoBuffer,0,sizeof(echoBuffer));

    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        die("recv() failed");

    // Get the vector of strings in the HTTP
    vector<string> http = parse(echoBuffer);
    if(http[0] == "GET")
    {
       // char *stream = get_file_as_bytes(http[1]);
       if(send_file(http[1],clntSocket));
       else
       {
           send(clntSocket, "HTTP/1.0 404 NOT FOUND!\r\n", strlen("HTTP/1.0 404 NOT FOUND!\r\n"), 0);
       }
    }
    else if(http[0]=="POST")
    {

        int bytesrcvd;
        char bfr[RCVBUFSIZE],rcvbfr[RCVBUFSIZE];
        memset(bfr,0,sizeof(bfr));

        send(clntSocket, "HTTP/1.0 200 OK", strlen("HTTP/1.0 200 OK"), 0);

        int file_size=0,stat;
        do{
              stat = recv(clntSocket, &file_size, sizeof(int),0);
        }while(stat<0);

        cout<<"The file size is "<<file_size<<endl;
        int total_bytes = 0,idx =0;
        while(total_bytes < file_size){
            bytesrcvd = recv(clntSocket,rcvbfr,RCVBUFSIZE-1,0);
            for(int i=0;i<bytesrcvd;i++){
                bfr[idx+i]=rcvbfr[i];
            }
            idx += bytesrcvd;
            total_bytes += bytesrcvd;
        }

        bfr[idx] = '\0';
        cout<<"server received "<<total_bytes<<" bytes\n";
        write_bytes_to_file(bfr,http[1],total_bytes);
    }
    else{
      cout<<"unknown method\n";
    }
    close(clntSocket);
}

class Server{
    public:
      int port;
      Server(int x){
          port = x;
      }

      void Setup(){
        int serverSocket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(serverSocket < 0){
            die("socket() failed");
        }

        ///2. bind socket to the port
        struct  sockaddr_in serverAddr;
        memset(&serverAddr,0,sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0){
            die("bind() error");
        }

        ///3. listen
        if(listen(serverSocket,MAXPENDING) < 0){
            die("listen() error");
        }

        cout<<"Server is listening at port "<<port<<endl;

        /// wait for requests
        while(1){

            int clntSocket;
            struct sockaddr_in clnt;
            socklen_t clntlen = sizeof(clnt);
            if((clntSocket = accept(serverSocket,(struct sockaddr*)&clnt,&clntlen)) <0){
                    die("Accept failed()");
            }
            std::thread (HandleClient,clntSocket).detach();
        }
      }

};

int main(int args,char *argv[])
{

    ///1.Create a TCP socket using socket().
    if(args != 2){
        printf("error!");
    }
    unsigned short serverPort = atoi(argv[1]);
    Server server = Server(serverPort);
    server.Setup();
    return 0;
}
