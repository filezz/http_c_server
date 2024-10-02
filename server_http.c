#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h>
#include<unistd.h>
#include<sys/stat.h>
#include<string.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<fcntl.h>

#define PORT 6969
#define BUFFER_SIZE 4096

void handle_client(int client_fd);
char *file_ext(const char *url);
const char *mime_type(const char *extension);
const char *ext(const char *file);
void http_resp(const char *file,const char *extension,char *buffer,size_t *buffer_len);

int main(int argc, char *argv[])
{
    int server_fd ;
    struct sockaddr_in server_addr;

    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
        perror("error with socket"),exit(EXIT_FAILURE);


    int reuse =1 ; //reusing socket adddr 
    if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse)) < 0)
        perror("Error with reusing ADDR"),close(server_fd),exit(EXIT_FAILURE);

    
    printf("----STAGE 1 SOCKET FUNCTION DONE-----\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY ;
    server_addr.sin_port = htons(PORT) ;
    
    if(bind(server_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
        perror("error with bind"),close(server_fd),exit(EXIT_FAILURE);

    printf("--------DONE WITH STAGE 2 BIND FUNCTION ------\n");

    if(listen(server_fd,10) < 0)
        perror("error with listen function"),close(server_fd),(EXIT_FAILURE);
    
    printf("-------DONE WITH STAGE 3 LISTEN FUNCTION --------\n");

    printf("-----SERVER IS LISTENINGN ON PORT :%d --------\n" , PORT); 

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd;

    while(1){ 

        client_fd = accept(server_fd,(struct sockaddr *)&client_addr,&client_len);
        if(client_fd < 0){
            perror("error with accept");
            continue;
        }
    handle_client(client_fd);        
    }
   
    
    return 0;
    }
void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t valread = recv(client_fd,buffer,BUFFER_SIZE,0);
    if (valread < 0)
        perror("Error with reading from client"),close(client_fd),exit(EXIT_FAILURE);
    
    buffer[valread] = '\0';
    
    char *file = file_ext(buffer);
    if(file==NULL)
        perror("Invalid file req\n"),close(client_fd),exit(EXIT_FAILURE);    

    const char *extension = ext(file);
    if(extension == NULL)
        perror("Invalid extension\n"),free(file),close(client_fd),exit(EXIT_FAILURE);
    
    char response[BUFFER_SIZE];
    size_t response_len ;
    http_resp(file,extension,response,&response_len);
    if(send(client_fd,response,response_len,0) < 0 )
        perror("Error with sending to the client"); 
    // ------ If in order to be able to print response, %s expects a null terminated pointer
    if(response_len < BUFFER_SIZE)//introduced in order to eliminate the posibility of a segfault after calling printf function  
        response[response_len] ='\0';
    else 
        response[BUFFER_SIZE-1] = '\0';
        

    printf("Response sent :%s \n",response); 
    free(file);
    close(client_fd);

}

char *file_ext(const char *url)
{
    char *arg_cpy = strdup(url);
    if(arg_cpy == NULL) 
        return NULL;

    char *tok = strtok(arg_cpy," ");
    tok = strtok(NULL," ");

    if(!tok)
    {
        free(arg_cpy); 
        return NULL;
    }
    if(*tok == '\0')
    {
        free(arg_cpy);
        return NULL;
    }
    tok = tok +1 ;

    char *result  = strdup(tok); //returning directly the token can lead to memory leaks (not checking if token is valid)
    if(result == NULL){
        perror("Strdup() failed ");
        free(arg_cpy);
        return NULL;
    }
    free(arg_cpy);
    return result; 
}

const char *ext(const char *file)
{
    const char *dot = strrchr(file,'.');
    if(!dot ||dot == file)
        return NULL;
    return dot+1;
}

const char *mime_type(const char *extension)
{
 
    if(strcasecmp(extension,"txt") == 0)
        return "text/plain";
    else if(strcasecmp(extension,"html") == 0 || strcasecmp(extension,"htm") == 0)
        return "text/html";
    else if(strcasecmp(extension,"jpg") == 0 || strcasecmp(extension, "jpeg") == 0)
        return "image/jpg";
    else if(strcasecmp(extension,"png") ==0)
        return "image/png";
    else 
        return "application/octet-stream";
}


void http_resp(const char *file,const char *extension,char *response,size_t *response_len)
{
    const char *mime = mime_type(extension);
    char header[BUFFER_SIZE];//printf("Ext :%s ",extension),printf("Mime type:%s ",mime);
    *response_len = 0;
    
    snprintf(header,BUFFER_SIZE,
            "HTTP/1.0 200 OK\r\n"
            "Server: webserver-c\r\n"
            "Content-Type:%s\r\n"
            "\r\n",mime);

    int file_fd = open(file,O_RDONLY);
    if(file_fd < 0){

        snprintf(response,BUFFER_SIZE,
                "HTTP/1.0 404 Not Found\r\n"
                "Server: webserver-c \r\n"
                "Content-Type:text/plain \r\n"
                "\r\n404 NOT FOUND \r\n");
        *response_len = strlen(response);
        return;
    }

    struct stat file_info;
    fstat(file_fd , &file_info);
    off_t file_size = file_info.st_size;

    //cpy info to buffer;

    memcpy(response,header,strlen(header)); 
    *response_len += strlen(header); 

    size_t bytes_copied; 
    while( *response_len < BUFFER_SIZE && (bytes_copied = read(file_fd, response + *response_len, BUFFER_SIZE - *response_len)) > 0) 
        *response_len += bytes_copied ; 


    close(file_fd) ;
}
