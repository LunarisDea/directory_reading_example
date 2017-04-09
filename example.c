#include <fnmatch.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dirent.h>

#define BACKLOG (10)
#define DIRECTORY_LISTING_MAX_CHAR 1013

void serve_request(int, char*);

char * request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";

char * request_str_gif = "HTTP/1.0 200 OK\r\n"
        "Content-type: image/gif; charset=UTF-8\r\n\r\n";

char * request_str_jpeg = "HTTP/1.0 200 OK\r\n"
        "Content-type: image/jpeg; charset=UTF-8\r\n\r\n";
		
char * request_str_png = "HTTP/1.0 200 OK\r\n"
        "Content-type: image/png; charset=UTF-8\r\n\r\n";
		
char * request_str_jpg = "HTTP/1.0 200 OK\r\n"
        "Content-type: image/jpg; charset=UTF-8\r\n\r\n";
		
char * request_str_txt = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/plain; charset=UTF-8\r\n\r\n";

char * request_str_pdf = "HTTP/1.0 200 OK\r\n"
        "Content-type: application/pdf; charset=UTF-8\r\n\r\n";
		
char * request_str_404 = "HTTP/1.0 404 NOT FOUND\r\n"
 		"Content-type: text/html; charset=UTF-8\r\n\r\n";


char * index_hdr = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html>"
        "<title>Directory listing for %s</title>"
"<body>"
"<h2>Directory listing for %s</h2><hr><ul>";

// snprintf(output_buffer,4096,index_hdr,filename,filename);


char * index_body = "<li><a href=\"%s\">%s</a>";

char * index_ftr = "</ul><hr></body></html>";

char * index_full=  "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n""<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html>"
        "<title>Directory listing for %s</title>"
"<body>"
"<h2>Directory listing for %s</h2><hr><ul>" "<li><a href=\"%s\">%s</a>" "</ul><hr></body></html>", filename, filename, directory_contents_str, directory_contents_str;

/* char* parseRequest(char* request)
 * Args: HTTP request of the form "GET /path/to/resource HTTP/1.X" 
 *
 * Return: the resource requested "/path/to/resource"
 *         0 if the request is not a valid HTTP request 
 * 
 * Does not modify the given request string. 
 * The returned resource should be free'd by the caller function. 
 */
char* parseRequest(char* request) {
  //assume file paths are no more than 256 bytes + 1 for null. 
  char *buffer = malloc(sizeof(char)*257);
  memset(buffer, 0, 257);
  
  if(fnmatch("GET * HTTP/1.*",  request, 0)) return 0; 

  sscanf(request, "GET %s HTTP/1.", buffer);
  return buffer; 
}


char* get_directory_contents(char* directory_path)
{
  char* directory_listing = NULL;
  
  // open directory path up 
  DIR* path = opendir(directory_path);

  // check to see if opening up directory was successful
  if(path != NULL)
  {
      directory_listing = (char*) malloc(sizeof(char)*DIRECTORY_LISTING_MAX_CHAR);
      directory_listing[0] = '\0';

      // stores underlying info of files and sub_directories of directory_path
      struct dirent* underlying_file = NULL;

      // iterate through all of the  underlying files of directory_path
      while((underlying_file = readdir(path)) != NULL)
      {
          strcat(directory_listing, underlying_file->d_name);
          strcat(directory_listing, "\n");
      }
      
      closedir(path);
  }

  return directory_listing;
}



void serve_request(int client_fd, char * directory){
  int read_fd;
  int bytes_read;
  int file_offset = 0;
  char client_buf[4096];
  char send_buf[4096];
  char filename[4096];
  char find_index[4096];
  char * requested_file;
  char requested_file_parse[4096];
  char index[4096];
  char file404[4096];
  memset(client_buf,0,4096);
  memset(filename,0,4096);
  
  while(1){

    file_offset += recv(client_fd,&client_buf[file_offset],4096,0);
    if(strstr(client_buf,"\r\n\r\n"))
      break;
  }
  requested_file = parseRequest(client_buf);
  
  //parse file
  int i=1;
  while(i<=strlen(directory)){
      i++;
  }
  int j=i+1;
  for(i=j; i<strlen(requested_file); i++){
      requested_file_parse[i-j]=requested_file[i];
  }
  printf("Original requested file: %s\n", requested_file_parse); //testing
  printf("length of request: %d\n", strlen(requested_file_parse));
  
  strncpy(&filename[0],directory,4096);
  filename[strlen(directory)]='/';
    
  for(i=strlen(filename); j<strlen(requested_file); i++, j++){
    filename[i]=requested_file[j];
  }
  
  strncpy(&index[0], "index.html", 4096);
  strncpy(&find_index[0],filename, 4096);
  strncpy(&find_index[strlen(filename)], index, 4096-strlen(filename)); 
  printf("Find index is: %s\n", find_index);
  
  struct stat file_stat;

  //if no one file is requested
  if(strstr(requested_file_parse,".")==NULL){
  	//is there an index file present
  if (stat(find_index, &file_stat) != 0) {
  	//index does not exist
    printf("%s does not exist!  Program exiting\n", find_index);

    stat(filename, &file_stat);

      	if (S_ISDIR(file_stat.st_mode)) {
   			printf("%s is a directory\n", filename);
   			char* directory_contents_str = get_directory_contents(filename); 
  			if(directory_contents_str != NULL)
  			{
  				// is a directory
  				send(client_fd,index_full,strlen(index_full),0);
   				printf("Contents of %s are:\n", filename);
    			printf("%s\n", directory_contents_str); 
  			}
  			else
  			{
   				fprintf(stderr, "Error reading contents of %s\n", filename);
    			return;
  			}
  		} 
  		else {
   			printf("%s is NOT a directory!  Program exiting\n", filename);
    		return;
  		}
    return;
  }
  else{
    //index does exist
      //strncpy(&find_index[0], requested_file, 4096);
      //strncpy(&find_index[strlen(requested_file)], index, 4096-strlen(requested_file)); 
      printf("The index page is: %s\n", find_index);
       strncpy(&requested_file[0], find_index, 4096);
      read_fd = open(find_index,0,0);
      //printf("
  }
  }
  else{
    read_fd = open(filename,0,0);
  }


  printf("File to open: %s\n", filename); //testing 
  printf("New requested file:%s\n", requested_file); //testing

  
  for(i=0; i<100; i++){
  	if(read_fd<0){
  		send(client_fd, request_str_404, strlen(request_str_404), 0);
  		break;
  	}
      else if(requested_file[i]=='.'){
          if(requested_file[i+1]=='h'){
              send(client_fd,request_str,strlen(request_str),0);
              break;
          }
          else if(requested_file[i+1]=='g'){
              send(client_fd,request_str_gif,strlen(request_str_gif),0);
              break;
          }
          else if(requested_file[i+1]=='j'){
			  if(requested_file[i+3]=='e'){
				send(client_fd,request_str_jpeg,strlen(request_str_jpeg),0);
				break;
			  }
			  else{
				send(client_fd,request_str_jpg,strlen(request_str_jpg),0);
				break;
			  }
          }
		  else if(requested_file[i+1]=='p'){
			  if(requested_file[i+2]=='n'){
				send(client_fd,request_str_png,strlen(request_str_png),0);
				break;
			  }
			  else{
				send(client_fd,request_str_pdf,strlen(request_str_pdf),0);
				break;
			  }
		  }
		  else if(requested_file[i+1]=='t'){
			  send(client_fd,request_str_txt,strlen(request_str_txt),0);
			  break;
		  }
      }
  }
  //send(client_fd,request_str_gif,strlen(request_str),0);
  // take requested_file, add a . to beginning, open that file
  
  if(read_fd<0){
	strncpy(&file404[0], "WWW/404.html", 4096);
	printf("Opening 404 at: %s\n", file404);
	read_fd=open(file404, 0, 0);
	printf("Opened.\n");
  }

  while(1){
    bytes_read = read(read_fd,send_buf,4096);
    if(bytes_read == 0){
      break;
	}
	
    send(client_fd,send_buf,bytes_read,0);
  }
  memset(&filename[0], 0, sizeof(filename));
  memset(&requested_file[0], 0, sizeof(requested_file));
  memset(&requested_file_parse[0], 0, sizeof(requested_file_parse));
  memset(&find_index[0], 0, sizeof(find_index));
  memset(&file_stat, 0, sizeof(file_stat));

  close(read_fd);
  close(client_fd);
  return;
}

/* Your program should take two arguments:
 * 1) The port number on which to bind and listen for connections, and
 * 2) The directory out of which to serve files.
 */
int main(int argc, char** argv) {
    /* For checking return values. */
    int retval;

    /* Read the port number from the first command line argument. */
    int port = atoi(argv[1]);
    char * directory = argv[2];

	printf("Directory: %s\n", directory); //testing

    /* Create a socket to which clients will connect. */
    int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    /* A server socket is bound to a port, which it will listen on for incoming
     * connections.  By default, when a bound socket is closed, the OS waits a
     * couple of minutes before allowing the port to be re-used.  This is
     * inconvenient when you're developing an application, since it means that
     * you have to wait a minute or two after you run to try things again, so
     * we can disable the wait time by setting a socket option called
     * SO_REUSEADDR, which tells the OS that we want to be able to immediately
     * re-bind to that same port. See the socket(7) man page ("man 7 socket")
     * and setsockopt(2) pages for more details about socket options. */
    int reuse_true = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true,
                        sizeof(reuse_true));
    if (retval < 0) {
        perror("Setting socket option failed");
        exit(1);
    }

    /* Create an address structure.  This is very similar to what we saw on the
     * client side, only this time, we're not telling the OS where to connect,
     * we're telling it to bind to a particular address and port to receive
     * incoming connections.  Like the client side, we must use htons() to put
     * the port number in network byte order.  When specifying the IP address,
     * we use a special constant, INADDR_ANY, which tells the OS to bind to all
     * of the system's addresses.  If your machine has multiple network
     * interfaces, and you only wanted to accept connections from one of them,
     * you could supply the address of the interface you wanted to use here. */
    
   
    struct sockaddr_in6 addr;   // internet socket address data structure
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port); // byte order is significant
    addr.sin6_addr = in6addr_any; // listen to all interfaces

    
    /* As its name implies, this system call asks the OS to bind the socket to
     * address and port specified above. */
    retval = bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    if(retval < 0) {
        perror("Error binding to port");
        exit(1);
    }

    /* Now that we've bound to an address and port, we tell the OS that we're
     * ready to start listening for client connections.  This effectively
     * activates the server socket.  BACKLOG (#defined above) tells the OS how
     * much space to reserve for incoming connections that have not yet been
     * accepted. */
    retval = listen(server_sock, BACKLOG);
    if(retval < 0) {
        perror("Error listening for connections");
        exit(1);
    }

    while(1) {
        /* Declare a socket for the client connection. */
        int sock;
        char buffer[256];

        /* Another address structure.  This time, the system will automatically
         * fill it in, when we accept a connection, to tell us where the
         * connection came from. */
        struct sockaddr_in remote_addr;
        unsigned int socklen = sizeof(remote_addr); 

        /* Accept the first waiting connection from the server socket and
         * populate the address information.  The result (sock) is a socket
         * descriptor for the conversation with the newly connected client.  If
         * there are no pending connections in the back log, this function will
         * block indefinitely while waiting for a client connection to be made.
         * */
        sock = accept(server_sock, (struct sockaddr*) &remote_addr, &socklen);
        if(sock < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        /* At this point, you have a connected socket (named sock) that you can
         * use to send() and recv(). */

        /* ALWAYS check the return value of send().  Also, don't hardcode
         * values.  This is just an example.  Do as I say, not as I do, etc. */
        serve_request(sock, directory);

        /* Tell the OS to clean up the resources associated with that client
         * connection, now that we're done with it. */
        close(sock);
    }

    close(server_sock);
}
