#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <map>

#include <ctype.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSIZE 2048
#define FILESIZE 100
#define CAPACITY 20

using namespace std;

typedef struct client{
	int fd;
	//char *ip;
	//unsigned short port;
	string name;
	string filename[FILESIZE];
}INFO;

typedef struct type{
	char buf[BUFSIZE];
	int now;
	int size;
	int byte;
	bool send;
	//bool done;
	char file[1024];
	char name[50];
}TCP;

multimap<string,string> mapfile;
multimap<string,string>::iterator iter;

void shutdown(fd_set *, int , int );
void saywelcome(fd_set *,fd_set *, int , int , int , INFO *);
void command(fd_set * , fd_set * , int , int , int , INFO *);
void command_put(fd_set * , int , INFO * ,char *,int);
void command_sleep(int , INFO *, char *);

int main(int argc, char *argv[]){
	if(argc != 2){
		cout<<"usage: ./server <SERVER_PORT>";
		exit(1);
	}
	unsigned short port;
	int sockfd=0;
	int maxfd;
	port=atoi(argv[1]);
	sockfd = socket(PF_INET , SOCK_STREAM , 0);
	if(sockfd < 0){
		cout<<"Fail to create a socket";
		exit(1);
	}

	struct sockaddr_in serverinfo;
	serverinfo.sin_family = PF_INET;
	serverinfo.sin_addr.s_addr = INADDR_ANY;
	serverinfo.sin_port = htons(port);

	bind(sockfd , (struct sockaddr *) &serverinfo , sizeof(serverinfo));
	listen(sockfd,CAPACITY);

	fd_set rfds,wfds,mas,wmas;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&wmas);
	FD_ZERO(&mas);
	FD_SET(0,&mas);
	FD_SET(sockfd,&mas);

	char buf[BUFSIZE];
	INFO info[CAPACITY];
	for(int i=0 ; i<CAPACITY ;i++){
		info[i].fd=-1;
		info[i].name="anonymous";
		for(int j=0 ; j<FILESIZE;j++){
			info[i].filename[j]="fuckleo";
		}
	}
	maxfd=(sockfd>0)?sockfd:0;

	while(true){
		rfds=mas;
		wfds=wmas;
		if(select(maxfd+1 , &rfds , &wfds ,NULL ,NULL)<0){
			cout<<"Fail to select";
			exit(1);
		}
		for(int i=0 ; i<=maxfd ; i++){
			if(FD_ISSET(i , &rfds)){
				memset(buf , 0 , BUFSIZE);
				if(i==0){
					fgets(buf , BUFSIZE , stdin);
					if(strcmp(buf , "exit\n")==0){
						shutdown(&mas , maxfd , sockfd);
						exit(0);
					}
				}
				else if(i==sockfd){
					sockaddr_in clientinfo;
					int clientfd;
					socklen_t len = sizeof(clientinfo);
					if( (clientfd=accept(sockfd , (struct sockaddr *) &clientinfo , &len) ) < 0 ){
						cout<<"Fail to accept";
						exit(1);
					}
					int pos,tmp;
					for(int j=3;j<CAPACITY;j++){
						if(info[j].fd == -1 && j!=sockfd){
							pos=j;
							break;
						}
						tmp=j;
					}
					if(tmp!=CAPACITY-1){
						info[pos].fd=clientfd;
						maxfd = (clientfd > maxfd)? clientfd : maxfd;
						cout<<"new client from "<<clientfd<<"   client's pos is "<<pos<<endl;
						FD_SET(clientfd , &mas);
						FD_SET(clientfd , &wmas);
						int flag=fcntl(clientfd , F_GETFL , 0);
						fcntl(clientfd , F_SETFL , flag|O_NONBLOCK);
						char temp2[BUFSIZE];
						memset(temp2,0,BUFSIZE);
						usleep(10000);	
						recv(clientfd , temp2 , BUFSIZE , 0);
						temp2[strlen(temp2)]='\0';
						send(clientfd , temp2 , BUFSIZE , 0);
						usleep(10000);
						cout<<temp2<<endl;;
						info[pos].name=temp2;
						saywelcome(&wmas ,&mas , maxfd , clientfd , sockfd ,&info[0]);
					}
				}
				if(i != sockfd && i>2)
					command(&wmas,&mas , maxfd , i , sockfd , &info[0]);
			}
		}
	}
	close(sockfd);
	return 0;
}

void command(fd_set *wmas , fd_set *mas, int maxfd , int fd , int sockfd , INFO *info){
	char buf[BUFSIZE];
	TCP temp;
	memset(buf , 0 ,BUFSIZE);
	char *str;
	int pos;
	if(recv(fd , &temp , sizeof(type) , 0) <= 0){
		//if(errno!=EWOULDBLOCK){
			cout<<"Client leaves , fd : "<<fd<<endl;
			for(int i=0 ; i<CAPACITY ; i++){
				if(info[i].fd == fd){
					pos=i;
					break;
				}
			}
			//cout<<"pos= "<<pos<<endl;
			sprintf(buf , "[Server] %s from %d is offline\n" , info[pos].name.c_str(),info[pos].fd);
			memcpy(temp.buf , buf , strlen(buf));
			temp.buf[strlen(buf)]='\0';
			temp.now=-1;
			for(int i=0 ; i<CAPACITY ; i++){
				if(i!=pos && info[i].fd>=0)
					send(info[i].fd , &temp.buf , BUFSIZE , 0);
			}
			info[pos].fd=-1;
			info[pos].name="anonymous";
			for(int i=0;i<FILESIZE;i++)
				info[pos].filename[i]="fuckleo";
			FD_CLR(fd , wmas);
			FD_CLR(fd , mas);
			close(fd);
		//}
	}
	else{
		if( strncmp(temp.buf , "/put " ,5)==0 ){
			str=temp.buf+5;
			command_put(wmas , fd , &info[0] , str, sockfd);
		}
		else if( strncmp(temp.buf , "/sleep " ,7)==0 ){
			str=temp.buf+6;
			command_sleep(fd , &info[0] , str);
		}
		else if(strcmp(temp.buf , "/exit\n") == 0 || strcmp(temp.buf , "exit\r\n") == 0){
			cout<<"over";
			info[fd].fd=-1;
			info[fd].name="anonymous";
			FD_CLR(fd , wmas);
			FD_CLR(fd , mas);
			close(fd);
		}
		else if(strcmp(temp.buf, "\n" )==0 || strcmp(temp.buf,"\r\n")==0 ){
			/*do nothing*/
		}
		else{
			char error[]="Error command.\n";
			memset(temp.buf , 0 ,BUFSIZE);
			memcpy(temp.buf,error,strlen(error));
			temp.buf[strlen(error)]='\0';
			temp.now=-1;
			send(fd , &temp.buf , BUFSIZE , 0);
		}
	}
}

void command_put(fd_set *wmas ,int fd , INFO *info , char *str ,int sockfd){
	string message=str;
	string tmp;
	bool flag=false;
	char buf[BUFSIZE];
	int i;
	stringstream ss(message);
	ss>>tmp;
	message=tmp;
	FILE *file;
	int byte;
	TCP r,*temp;
	bool setup=false;
	int count=0;
	tmp=info[fd].name;
	tmp+=" ";
	tmp+=message;
	message=tmp;
	cout<<"filename: "<<message<<endl;
	file=fopen(message.c_str(),"wb+");
	cout<<"start to receive file from "<<fd<<endl;
	while(true){
		byte=read(fd , &r , sizeof(type) );
		if(byte<=0)
			continue;
		if(!setup){
			temp=new TCP[r.size];
			for(int i=0;i<r.size;i++){
				temp[i].send=false;
				//temp[i].done=false;
				memcpy(temp[i].file,message.c_str(),message.length());
				temp[i].file[message.length()]='\0';
				temp[i].now=i;
			}
			setup=true;
			cout<<"0's filename : "<<temp[0].file<<endl;
			cout<<"last filename : "<<temp[r.size-1].file<<endl;
		}
		/*cout<<"bbbbbbbb maybe"<<byte<<endl;
		cout<<"receive maybe "<<r.now<<endl;
		cout<<"byte maybe : "<<r.byte<<endl;*/
		/*if(r.byte<BUFSIZE)
			r.buf[r.byte]='\0';*/
		if(temp[r.now].send==false){
			//cout<<"receive "<<r.now<<endl;
			//cout<<"byte size : "<<r.byte<<endl;
			temp[r.now].send=true;
			memcpy(temp[r.now].buf,r.buf,r.byte);
			if(r.byte<BUFSIZE)
				temp[r.now].buf[r.byte]='\0';
			temp[r.now].byte=r.byte;
			temp[r.now].size=r.size;
			//temp[r.now].now=r.now;
			//cout<<temp[r.now].buf<<endl;
			count++;
		}
		if(count==r.size){
			cout<<"upload success!\n#####################\n";
			for(int i=0;i<r.size;i++){
				fwrite(temp[i].buf , sizeof(char) , temp[i].byte , file);
				//cout<<temp[i].buf;
			}
			break;
		}
		//byte=fwrite(buf , sizeof(char) , sizeof(buf) ,file);
	}




	int packet,size,last;
	fseek(file , 0 ,SEEK_END);
	size=ftell(file);
	fseek(file , 0 ,SEEK_SET);
	if( (size%(BUFSIZE))!=0 ){
		packet=(size/(BUFSIZE))+1;
		last=size%(BUFSIZE);
	}
	else{
		packet=(size/(BUFSIZE));
		last=2048;
	}
	cout<<"===================="<<endl;
	cout<<message.c_str()<<endl;
	for(int i=0;i<FILESIZE;i++){
		if(info[fd].filename[i]=="fuckleo"){
			info[fd].filename[i]=message;
			cout<<"now info filename : "<<message<<endl;
			break;
		}
	}
	ss.str("");
	tmp.clear();
	ss.clear();
	ss<<message;
	message.clear();
	ss>>tmp>>message;
	mapfile.insert(pair<string,string>(tmp,message));
	cout<<"user name : "<<tmp<<" filename : "<<message<<endl;
	cout<<"size : "<<packet<<endl;
	delete [] temp;
	char **s;
	s=new char*[packet];
	for(int i=0;i<packet;i++)
		s[i]=new char[BUFSIZE];
	for(int i=0;i<packet;i++){
		byte=fread(s[i] , sizeof(char) , BUFSIZE , file);
		//s[i][BUFSIZE]='\0';
		if(byte!=BUFSIZE)
			cout<<"now "<<i<<" byte "<<byte<<endl;
		if(i==packet-1&&last!=2048)
			s[i][last]='\0';
	}
	
	for(int i=4;i<CAPACITY;i++){
		int turn=1;
		if(info[i].fd>=0){
			if(info[i].name==tmp&&info[i].fd!=fd){
				while(turn>0){
					cout<<"now send to fd : "<<i<<endl;
					char packet_size[BUFSIZE];

					memset(packet_size,0,BUFSIZE);
					sprintf(packet_size,"%d",packet);
					packet_size[strlen(packet_size)]='\0';
					byte=send(i,packet_size,BUFSIZE,0);
					cout<<"send byte "<<byte<<endl;
					cout<<strlen(packet_size)+1<<endl;
					
					memset(packet_size,0,BUFSIZE);
					sprintf(packet_size,"%d",last);
					packet_size[strlen(packet_size)]='\0';
					byte=send(i,packet_size,BUFSIZE,0);
					cout<<"send byte "<<byte<<endl;
					cout<<strlen(packet_size)+1<<endl;
					
					memset(packet_size,0,BUFSIZE);
					sprintf(packet_size,"%s",message.c_str());
					packet_size[strlen(packet_size)]='\0';
					byte=send(i,packet_size,BUFSIZE,0);
					cout<<"send byte "<<byte<<endl;
					cout<<strlen(packet_size)+1<<endl;

					bool check=false;
					for(int j=0;j<packet;j++){
						if(FD_ISSET(i , wmas)){
							if(j!=packet-1){
								byte=send(i , s[j] , BUFSIZE , 0);
								usleep(10000);
								//if(byte<0)
									//cout<<"send byte "<<byte<<" packet "<<j<<endl;
							}
							else{
								byte=send(i , s[j] , BUFSIZE , 0);
								usleep(10000);
								//if(byte<0)
									//cout<<"send byte "<<byte<<" packet "<<j<<endl;
							}
						}
						else{
							cout<<"@#&^$*&@#^$@*&#^$&@*#^$*@&^$*#@^";
							j--;
						}
					}
					/*while(now_read<packet){
						memset(buf , 0 , BUFSIZE);
						//fseek(file,now_read,SEEK_SET);
						//cout<<"now seek set to "<<now_read<<endl;
						fread(buf , sizeof(char) , BUFSIZE-1 , file);
						buf[BUFSIZE]='\0';
						byte=send(i , buf , strlen(buf) , 0);
						if(byte>0)
							cout<<"file send "<<byte<<" success\n";
						now_read+=byte;
					}*/
					turn--;
				}
				char b[]="done\n";
				b[strlen(b)]='\0';
				byte=send(i,b,BUFSIZE,0);
				cout<<"send byte "<<byte<<endl;
			}
		}
		usleep(50000);
	}
	for(int i=0;i<packet;i++)
		delete [] s[i];
	delete [] s;
	fclose(file);
}

void command_sleep(int fd , INFO *info , char *str){
	/*string temp=str;
	int second;
	istringstream ss(str);
	ss>>second;
	cout<<"now to sleep "<<second<<" seconds";
	for(int i=1;i<=second;i++){
		cout<<"Sleep "<<i<<endl;
		sleep(1);
	}
	cout<<"Client wakes up\n";*/
}

void saywelcome(fd_set *wmas ,fd_set *mas,int maxfd ,int clientfd , int sockfd ,INFO *info){
	char buf[BUFSIZE];
	TCP temp;
	memset(buf , 0 ,BUFSIZE);
	int byte;
	for(int i2=0 ; i2<=CAPACITY ; i2++){
		if(info[i2].fd == clientfd){
			sprintf(buf , "Welcome to the dropbox-like server!:%s\n", info[i2].name.c_str());
			memcpy(temp.buf , buf , strlen(buf));
			temp.now=-1;
			temp.buf[strlen(buf)]='\0';
			if((byte=send(info[i2].fd , &temp.buf , BUFSIZE , 0))<0)
				cout<<"Fail to send buf";
			cout<<"send byte "<<byte<<endl;
			//for(int j2=3;j2<=CAPACITY;j2++){
				//if(info[j2].fd!=clientfd && info[j2].name==info[i2].name){
					//for(int k=0;k<FILESIZE;k++){
					for(iter=mapfile.begin();iter!=mapfile.end();iter++){
						string message;
						string tmp;
						if(iter->first==info[i2].name){
							FILE *file;
							message=iter->first;
							message+=" ";
							message+=iter->second;
							file=fopen(message.c_str(),"rb");
							stringstream ss;
							ss<<message;
							message.clear();
							ss>>tmp>>message;
							cout<<"check file name : "<<tmp<<" file : "<<message<<endl;
							int packet,size,last;
							fseek(file , 0 ,SEEK_END);
							size=ftell(file);
							fseek(file , 0 ,SEEK_SET);
							if( (size%(BUFSIZE))!=0 ){
								packet=(size/(BUFSIZE))+1;
								last=size%(BUFSIZE);
							}
							else{
								packet=(size/(BUFSIZE));
								last=2048;
							}
							cout<<"new packet : "<<packet<<" last : "<<last<<endl;
							char **s;
							s=new char*[packet];
							for(int i=0;i<packet;i++)
								s[i]=new char[BUFSIZE];
							for(int i=0;i<packet;i++){
								byte=fread(s[i] , sizeof(char) , BUFSIZE , file);
								if(byte!=BUFSIZE)
									cout<<"now "<<i<<" byte "<<byte<<endl;
								if(i==packet-1&&last!=2048)
									s[i][last]='\0';
							}
			
							cout<<"now send to fd : "<<i2<<endl;
							char packet_size[BUFSIZE];

							memset(packet_size,0,BUFSIZE);
							sprintf(packet_size,"%d",packet);
							packet_size[strlen(packet_size)]='\0';
							byte=send(i2,packet_size,BUFSIZE,0);
							cout<<"send byte "<<byte<<endl;
							cout<<strlen(packet_size)+1<<endl;
							
							memset(packet_size,0,BUFSIZE);
							sprintf(packet_size,"%d",last);
							packet_size[strlen(packet_size)]='\0';
							byte=send(i2,packet_size,BUFSIZE,0);
							cout<<"send byte "<<byte<<endl;
							cout<<strlen(packet_size)+1<<endl;
							
							memset(packet_size,0,BUFSIZE);
							sprintf(packet_size,"%s",message.c_str());
							packet_size[strlen(packet_size)]='\0';
							byte=send(i2,packet_size,BUFSIZE,0);
							cout<<"send byte "<<byte<<endl;
							cout<<strlen(packet_size)+1<<endl;

							bool check=false;
							for(int j=0;j<packet;j++){
								if(FD_ISSET(i2 , wmas)){
									if(j!=packet-1){
										byte=send(i2 , s[j] , BUFSIZE , 0);
										usleep(10000);
										if(byte<0)
											cout<<"send byte "<<byte<<" packet "<<j<<endl;
									}
									else{
										byte=send(i2 , s[j] , BUFSIZE , 0);
										usleep(10000);
										if(byte<0)
											cout<<"send byte "<<byte<<" packet "<<j<<endl;
									}
								}
								else{
									cout<<"@#&^$*&@#^$@*&#^$&@*#^$*@&^$*#@^";
									j--;
								}
							}
							char b[]="done\n";
							byte=send(i2,b,BUFSIZE,0);
							cout<<"send byte "<<byte<<endl;
							for(int i=0;i<packet;i++)
								delete [] s[i];
							delete [] s;
							fclose(file);
						}
					}	
					//}
				//}
			//}
		}
	}
}
void shutdown(fd_set *mas,int maxfd, int sockfd){
	char buf[BUFSIZE];
	memset(buf , 0 ,BUFSIZE);
	TCP temp;
	for(int i=3 ; i<=maxfd ;i++){
		if(FD_ISSET(i , mas) && i!=sockfd){
			sprintf(buf , "Server is offline from %d\n" , sockfd);
			memcpy(temp.buf,buf,strlen(buf));
			temp.now=-1;
			temp.buf[strlen(buf)]='\0';
			if(send(i , &temp.buf , BUFSIZE, 0)<0){
				cout<<"Fail to send";
				exit(1);
			}
			close(i);
			FD_CLR(i , mas);
		}
	}
}
