#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

#define RCVBUFSIZE 1000000
    /*
        Roadmap:
        1. create socket using socket()
        2. connect to the server connect ()
        3. send() and recv()
        4. close connection
    */
void die(char *s){
     perror(s); /// prints a descriptive error message to stderr. First the string str is printed, followed by a colon then a space.
     exit(1);
}
void write_bytes_to_file(char *bytes,string filename,int read_size){
  FILE *file = fopen(filename.c_str(),"w");
  fwrite(bytes,1,read_size, file);
}
void append_bytes_to_file(char *bytes,string filename,int read_size){
  FILE *file = fopen(filename.c_str(),"a");
  fwrite(bytes,1,read_size, file);
}
bool okay(char *s){
  for(int i=0;i+1<strlen(s);i++){
      if(s[i] == 'O' && s[i+1]=='K') return true;
  }
  return false;
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
   int read_size = fread(buffer, 1, size, file);
   int packet_index = 1,stat;

    printf("Sending file size\n");
    do{
      stat = send(clntSocket, (void *)&size, sizeof(int),0);
    }while(stat<0);

     printf("Sending file\n");
    do{
      stat = send(clntSocket, buffer, size,0);
    }while (stat < 0);

   return true;
}
class Client{
  public:
    string method,filename;
    unsigned short int port;

    Client(char **arguments){
        method = arguments[1];
        filename = arguments[2];
        port = atoi(arguments[3]);
    }

    void sendRequest(){
          int clntSocket;
          /// 1. create a socket
          if((clntSocket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) <0){
                  die("socket() failed");
          }
          ///
          string tmp = method + " " + filename;
          char echoString[(int)tmp.length() + 1];
          strcpy(echoString, tmp.c_str());
          unsigned short int serverPort = port;
          struct sockaddr_in serverAddr;
          memset(&serverAddr,0,sizeof(serverAddr));
          serverAddr.sin_family = AF_INET;
          serverAddr.sin_port = htons(serverPort);  /// host to network short
          serverAddr.sin_addr.s_addr = htons(INADDR_ANY);/// converts the ip from the x.y.x.y format to binary

          ///2.connect

          if(connect(clntSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) <0){
              die("connection failed()");
          }

          ///3. send and receive
          /// send to server
            unsigned int len = strlen(echoString);
            if(send(clntSocket,echoString,len,0) != len){
               die("oops sent() b3tt 3add tany mn bytes yastaaaa");
            }

             int bytesrcvd,total_bytes = 0;
             char bfr[RCVBUFSIZE-1],rcvbfr[RCVBUFSIZE];
             memset(bfr,0,sizeof(bfr));
            if(method == "GET"){

              int idx = 0,total_bytes = 0;
              bytesrcvd = recv(clntSocket,rcvbfr,RCVBUFSIZE-1,0);
              printf("%s\n", rcvbfr);
              if(okay(rcvbfr)){
                int file_size=0,stat;
                do{
                      stat = recv(clntSocket, &file_size, sizeof(int),0);
                }while(stat<0);
                cout<<"The file size is "<<file_size<<endl;
                memset(rcvbfr,0,sizeof(rcvbfr));

                while(total_bytes < file_size){
                    memset(rcvbfr,0,sizeof(rcvbfr));
                    bytesrcvd = recv(clntSocket,rcvbfr,RCVBUFSIZE-1,0);
                    idx += bytesrcvd;
                    total_bytes += bytesrcvd;
                    bfr[idx] = '\0';
                    idx = 0;
                    cout<<"client received a packet : "<<bytesrcvd<<"bytes"<<endl;
                    if(total_bytes == 0)
                    write_bytes_to_file(rcvbfr,filename,bytesrcvd);
                    else
                    append_bytes_to_file(rcvbfr,filename,bytesrcvd);
                    int x = 1;
                    while(send(clntSocket, (void *)&x, sizeof(int),0)<0);

                }

                cout<<"client received "<<total_bytes<<" bytes\n";
              }
            }
           else if(method == "POST"){
                  ///recive from server response if accept to upload or not
                  int bytesrcvd;
                  char bfr[RCVBUFSIZE-1];
                  memset(bfr,0,sizeof(bfr));

                  if((bytesrcvd = recv(clntSocket,bfr,RCVBUFSIZE-1,0) ) <=0){
                     die("recv() failed");
                  }

                  if(okay(bfr)){
                      if(send_file(filename,clntSocket)){

                      }
                      else{
                        printf("error file doesnot exist\n");
                      }
                  }
           }
          ///4. close
          close(clntSocket);
    }
};
int main(int argc,char *argv[])
{
    /// msg to be send and port number
    /// port number , message
    if(argc != 4){
        fprintf(stderr,"error !\n"); ///Standard error stream
        exit(1);
    }
    Client client = Client(argv);
    client.sendRequest();


    return 0;
}
