#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include <pwd.h>
#include<string.h>
#include<string>
#include<time.h>
#include<termios.h>
#include<stack>
#define UP_KEY 1001
#define DOWN_KEY 1004
#define RIGHT_KEY 1003
#define LEFT_KEY 1002
#define TAB_KEY 9
#define BACKSP_KEY 127
#define ENTER_KEY 10
using namespace std;
/*
 ....................increase the buffer size of the char* onece done to be on the safe side............................
 * */
//-------------------------------------------- Global variables----------------- -----------------------
int mypaths_len;
char* mypaths;//will be taken from file
int myhome_len;
char* myhome;//home
int user_len;
char* myuser;//done
char* myhostname;//done
int host_name_len;
char* myps_u;//will be taken from file.
int myps_len;
char* myps_s;//will be taken from file.

void fill_param(){
	//PS for u and s and path is already written in the file.
	//username hostname and myhome needs to be read from the system.
	//------------------------------------- taking data from the systems ------------------------------
	//----- reading host name------------------------
	host_name_len=100;
	myhostname=(char*)malloc((host_name_len+1)*sizeof(char));
	if(gethostname(myhostname,host_name_len)==-1){
		printf("error in reading host name\n");
	}
	//------------------------------------------------
	//---- reading user name -------------------------
	struct passwd* temp=getpwuid(getuid());
	user_len=100;
	myuser=(char*)malloc((user_len+1)*(sizeof(char)));
	strcpy(myuser,temp->pw_name);
	//------------------------------------------------
	//----- reading home dir ------------------------
	myhome_len=100;
	myhome=(char*)malloc((myhome_len+1)*sizeof(char));
	strcpy(myhome,temp->pw_dir);
	//-----------------------------------------------
	//----------------------------------------reading from file ----------------------------------
	myps_len=100;
	myps_u=(char*)malloc((myps_len+1)*(sizeof(char)));
	myps_s=(char*)malloc((myps_len+1)*(sizeof(char)));
	mypaths_len=300;
	mypaths=(char*)malloc((mypaths_len+1)*(sizeof(char)));
	FILE* fp;
	char* line=NULL;
	size_t len=0;
	ssize_t read;
	fp=fopen(".myshellinfo","r");
	if(fp==NULL){
		printf("error in reading the file\n");
	}
	int line_number=0;	
	while((read=getline(&line,&len,fp))!=-1){
		//printf("retriving line %s",line);
		if(line_number==0)
			strcpy(myps_u,line);
		else if(line_number==1)
			strcpy(myps_s,line);
		else if(line_number==2)
			strcpy(mypaths,line);
		else
			break;

		line_number++;
	}
	fclose(fp);
	if(line)
		free(line);
	//----------------------done reading----------------------
	

	int tempp=strlen(mypaths);
	mypaths[tempp-1]='\0';
	tempp=strlen(myps_u);
	myps_u[tempp-1]='\0';
	tempp=strlen(myps_s);
	myps_s[tempp-1]='\0';
	//-------------setting the enviorment varable-------------
	setenv("PATH",mypaths,1);	//only this enviorment variable is required to set.
	setenv("HOME",myhome,1);
	setenv("USER",myuser,1);
	char* p=(char*)malloc(200*sizeof(char));
	//char *getcwd(char *buf, size_t size);
	p=getcwd(p,200);
	if(p==NULL){
		printf("error in reading present working directory\n");
		free(p);
	}else{
		setenv("PWD",p,1);
		free(p);
	}
	//--------------------------------------------------------	
	
}

//this method will be backbone of our program.
//moving the terminal to the background will not be done in this method we will assume the terminal is already in background.
//i.e the file discriptor of the terminal program will already be manneged.
int exe_fg(char* argv[],int input_fd,int output_fd){
	pid_t p=fork();
	if(p==0){//the code path of the child process.
		//now learn dup first.
		int a;
		a=dup2(input_fd,0);
		if(a==-1){
			printf("error in hand fd");
			return -1;
		}
		dup2(output_fd,1);
		if(a==-1){
			printf("error in hand fd 2");
			return -1;
		}
		a=execvp(*argv,argv);//this may through erro
		if(a==-1){
			printf("error in executing the command\n");
			exit(1);
			return -1;
		}
	}else{
		wait(NULL);
	}
	return 0;
}


//required to change he directory.
int my_cd(char** p){
 	int a=chdir(p[1]);
	if(a==-1){
		printf("error in changing directory\n");
		return -1;
	}
	//we have to again set the enviorment variable.
	char* pw=(char*)malloc(200*sizeof(char));
        pw=getcwd(pw,200);
        if(pw==NULL){
                printf("error in reading present working directory\n");
		free(pw);
        }else{
                setenv("PWD",pw,1);
		free(pw);
        }
	return 0;
 }

char getkey(){
	int ch;
	struct termios orig_t;
	struct termios new_t;
	tcgetattr(fileno(stdin),&orig_t);//setting the terminal in raw mode
	memcpy(&new_t,&orig_t,sizeof(struct termios));
	new_t.c_lflag &= ~(ICANON|ECHO);//bitwise and oprator.//remove not echo    ECHO
	new_t.c_cc[VTIME]=0;
	new_t.c_cc[VMIN]=0;
	tcsetattr(fileno(stdin),TCSANOW,&new_t);
	while((ch=fgetc(stdin))==EOF);
	tcsetattr(fileno(stdin),TCSANOW,&orig_t);
	return ch;
}
int get_key(){//the key 
	char ch[3];
	ch[0]=getkey();
	if(ch[0]==27){
		ch[1]=getkey();
		ch[2]=getkey();
		/*1001 for up 
		1002 for left 
		1003 for right and 
		1004 for down
		9 is for tap
		*/
		if(ch[2]==65){//up
			return 1001;
		}else if(ch[2]==66){//down
			return 1004;
		}else if(ch[2]==68){//left
			return 1002;
		}else if(ch[2]==67){//right
			return 1003;
		}
	}else{
		return ch[0];
	}

}

int howmany(char* s,char c){
	int len=strlen(s);
	int i;
	int count=0;
	for(i=0;i<len;i++){
		if(s[i]==c){
			count++;
		}
	}
	return count+1;
}
/*
char** mytoken_machine(char* s,char ch){
	int len=strlen(s);//all the token will be given following len.
	char* temp=new char[len];
	int i;
	while(s[i]!='\0'){

	}
}*/
void chr_print(char** a){
	int i=0;
	while(a[i]){
		printf("%s \n",a[i]);
		i++;
	}
}
//now our exevp needs to be rechecked
int main(){

	//handeling signals
	signal(SIGINT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	//filling the enviorment variables
	fill_param();
	char input_buffer[1024];
	int i;
	for(i=0;i<1023;i++){
		input_buffer[i]='a';
	}
	input_buffer[i]='\0';
	//printf("%s \n %s \n %s \n",mypaths,myps_s,myps_s);
	while(strcmp(input_buffer,"exit")!=0){
		//now we will take the input
		printf("%s ",myps_u);
		i=0;
		input_buffer[i]='\0';
		int a;
		while(1){//(a=get_key())!=ENTER_KEY
			a=get_key();
			if(a==TAB_KEY){

			}else if(a==UP_KEY){

			}else if(a==DOWN_KEY){

			}else if(a==BACKSP_KEY){
				printf("\b");
				printf(" ");
				printf("\b");
				i--;
			}else if(a==ENTER_KEY){
				printf("\n");
				break;
			}else{//the main condition to be handeled
				printf("%c",(char)a);
				input_buffer[i]=(char)a;
				i++;
			}
		}
		input_buffer[i]='\0';//this needs to be placed in the cache history stack.

		if(strcmp(input_buffer,"")==0){
			continue;
		}
		if(strcmp(input_buffer,"exit")==0){
			exit(1);
		}
		//input buffer ready here
		//----------------- tokening the stuff---------------
		int arr_num=howmany(input_buffer,' ');
		char** cmd=new char*[arr_num];
		char* token = strtok(input_buffer, " ");
		int j;
		int piece_len=strlen(input_buffer);
		for(j=0;j<arr_num;j++){
			cmd[j]=new char[piece_len];
		}  
		j=0;
   		while (token != NULL) { 
        	//printf("%s\n", token); 
			strcpy(cmd[j],token);
			j++;
        	token = strtok(NULL, " "); 
			
   		}
		   cmd[j]=0;
		//-------- token available in cmd ------------------------ 
		//--------- processing the token--------------------------
		if(strcmp(cmd[0],"cd")==0){
			//printf("executing the cd pahse\n");
			my_cd(cmd);
		}else if(strcmp(cmd[0],"exit")==0){
		
		}else{//normal command
			exe_fg(cmd,0,1);
		}
		//--------------------------------------------------------
		//---------deallocatin the space-------------------------
		for(j=0;j<arr_num;j++){
			delete cmd[j];
		}
		delete cmd;
		//---------------------------------------------------------  
	}

	return 0;
}
