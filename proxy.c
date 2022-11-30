/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
 
#define CHK_NULL(x) if((x) == NULL) exit(1);
#define CHK_ERR(err, s) if((err) == -1) { perror(s); exit(1); }
#define CHK_SSL(err) if((err) == -1) { ERR_print_errors_fp(stderr); exit(2); }


static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestlint_hdr_format = "%s %s HTTP/1.1\r\n";
static const char *requestlint_hdr_format_2 = "%s %s:%d HTTP/1.1\r\n";
static const char *endof_hdr = "\r\n";


static const char *connect_key = "CONNECT";

static const char *connection_key = "Connection";
static const char *user_agent_key= "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

/*
 * Function prototypes
 */
void parse_uri(char *HTTP,char *method, char *hostname,char *pathname,int *port, char *uri);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, char *hostname, int port);

void serve_static(int);
void serve_connect(int fd,int fd2,char *HTTP, char *hostname);
void build_http_header(char *http_header,char *method,char *hostname,char *path,int port,rio_t *client_rio);
void *thread(void *vargp);

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

        /* Check arguments */
    if (argc != 2) {
    fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
    exit(0);
    }

    listenfd = Open_listenfd(atoi(argv[1]));
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        Pthread_create(&tid,NULL,thread,(void *)connfd);
        // printf("%d %s\n",connfd,"Accepted");
    }


    exit(0);
}

void *thread(void *vargp){
    int connfd = (int)vargp;
    Pthread_detach(pthread_self());
    serve_static(connfd);
    Close(connfd);
}

void serve_static(int fd)
{
    char buf[MAXBUF];
    int port;
    int clientfd;
    char HTTP[MAXBUF];
    char HTTP_request[MAXBUF]="\0";
    char uri[MAXBUF];
    char hostname[MAXLINE], pathname[MAXLINE], method[MAXLINE];
    rio_t rio;
    struct sockaddr_in sockaddr;

    Rio_readinitb(&rio,fd);

    Rio_readlineb(&rio,HTTP,MAXBUF);
    printf("%s",HTTP);fflush(stdout);
    fflush(stdout);
    parse_uri(HTTP,method,hostname,pathname,&port,uri);
    build_http_header(HTTP_request,method,hostname,pathname,port,&rio);
    
    if (strcmp(method,connect_key)==0)
    {
        clientfd= Open_clientfd(hostname,port);
        ssl_connect()
    }
    else
    {
        clientfd= Open_clientfd(hostname,port);
        serve_connect(clientfd,fd,HTTP_request,hostname);
    }
    Close(clientfd);
    memset(&sockaddr,0, sizeof(sockaddr));
    format_log_entry(buf,&sockaddr,uri,hostname,port);
    printf("%s",buf);
    printf("Finished");
    fflush(stdout);
    
}   

void serve_connect(int fd,int fd2,char *HTTP, char *hostname)
{
    rio_t rio;
    char buf[MAXBUF];
    int n;

    Rio_readinitb(&rio,fd);
    Rio_writen(fd,HTTP,strlen(HTTP));
    printf("%s",HTTP);
    while((n=Rio_readlineb(&rio,buf,MAXBUF))!=0)
    {
        printf("buf == %s",buf); fflush(stdout);
        // Rio_writen(fd2,buf,n);
    };
}


void sslconnect()
{

    int err;
    int sd;
    struct sockaddr_in sa;
   
    /* SSL 관련 정보를 관리할 구조체를 선언한다. */
    SSL_CTX   *ctx;
    SSL     *ssl;
    X509                    *server_cert;
    char                    *str;
    char                    buf[4096];
    SSL_METHOD    *meth;
   
    /* 암호화 통신을 위한 초기화 작업을 수행한다. */
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    meth = SSLv3_client_method();
    ctx = SSL_CTX_new(meth);
    CHK_NULL(ctx);

}

void build_http_header(char *http_header,char *method,char *hostname,char *path,int port,rio_t *client_rio)
{
    char buf[MAXLINE],request_hdr[MAXLINE],other_hdr[MAXLINE],host_hdr[MAXLINE];
    /*request line*/
    if (strcmp(method,connect_key)==0)
    {
        sprintf(request_hdr,requestlint_hdr_format_2,method,hostname,port);
    }
    else
    {
        sprintf(request_hdr,requestlint_hdr_format,method,path);
    }
    /*get other request header for client rio and change it */
    while(Rio_readlineb(client_rio,buf,MAXLINE)>0)
    {
        if(strcmp(buf,endof_hdr)==0) break;/*EOF*/

        if(!strncasecmp(buf,host_key,strlen(host_key)))/*Host:*/
        {
            strcpy(host_hdr,buf);
            continue;
        }

        if(!strncasecmp(buf,connection_key,strlen(connection_key))
                &&!strncasecmp(buf,proxy_connection_key,strlen(proxy_connection_key))
                &&!strncasecmp(buf,user_agent_key,strlen(user_agent_key)))
        
        {
            strcat(other_hdr,buf);
        }
    }
    if(strlen(host_hdr)==0)
    {
        sprintf(host_hdr,host_hdr_format,hostname);
    }
    if (strcmp(method,connect_key)==0)
    {
        sprintf(http_header,"%s%s%s",
            request_hdr,
            host_hdr,
            endof_hdr);
        return;

    }
    sprintf(http_header,"%s%s%s%s%s%s%s",
            request_hdr,
            host_hdr,
            conn_hdr,
            prox_hdr,
            user_agent_hdr,
            other_hdr,
            endof_hdr);

    return ;
}

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
void parse_uri(char *HTTP,char *method, char *hostname,char *pathname,int *port, char * uri)
{
    *pathname = '/';
    
    *port = 80;
    
    char* pos = strstr(HTTP," ");
    *pos = '\0';
    sscanf(HTTP,"%s",method);
    pos++;

    char* pos4 = strstr(pos," ");
    *pos4 = '\0';
    sscanf(pos,"%s",uri);
    char* pos3 = strstr(pos,"//");
    pos = pos3!=NULL? pos3+2:pos;

    char* pos2 = strstr(pos,":");
    if(pos2!=NULL)
    {
        *pos2 = '\0';
        sscanf(pos,"%s",hostname);
        sscanf(pos2+1,"%d",port);
    }
    else
    {
        pos2 = strstr(pos,"/");
        if(pos2!=NULL)
        {
            *pos2 = '\0';
            sscanf(pos,"%s",hostname);
            *pos2 = '/';
            sscanf(pos2,"%s",pathname);
        }
        else
        {
            sscanf(pos,"%s",hostname);
        }
    }
    return;
}


/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, char *hostname, int port)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;
    struct hostent *hp;
    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));
    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */

    if ((hp = gethostbyname(hostname)) == NULL)
        return ;
    sockaddr->sin_family = AF_INET;
    memcpy(&sockaddr->sin_addr.s_addr ,hp->h_addr_list[0], hp->h_length);
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;
    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s\n", time_str, a, b, c, d, uri);
}

