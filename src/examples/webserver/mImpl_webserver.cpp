#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include "mImpl.h"

#include <vector>

#include "OFluxGenerate_webserver.h"

#define ERR_READ  -1
#define ERR_WRITE -2

int s;
struct sockaddr_in server_addr;
char *root;
int root_len;

fd_set read_fds;
int fd_max;
struct timeval select_timeout;

int socket_in_use[1024];

int suffixTest(const char *val, const char *suffix) {
  int len = strlen(val);
  int s_len = strlen(suffix);
  int i;
  
  for (i=0;i<s_len;i++) {
    if (val[len-i]!=suffix[s_len-i])
      return 0;
  }
  return 1;
}

inline char *makeHeaders(char *content, const char *close_hdr, int length) {
  return NULL;
}

void returnSocket(int socket) {
  socket_in_use[socket] = 0;
}

void closeSocket(int socket) {
  close(socket);
  if (socket > -1) {
    if (socket < 1024)
      socket_in_use[socket] = 2;
    else 
      printf("ERR, socket to large\n");
  }
}

void init(int argc, char **argv) {
  //int old_flags;
  
  for (int i=0;i<1024;i++)
    socket_in_use[i] = 2;

  s = socket(AF_INET,SOCK_STREAM,0);
  int val = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
    {
        perror("setsockopt: ");
        exit(1);
    }
 
  if (argc < 3) {
     fprintf (stderr, "Usage: %s <port-number> <root-dir>\n", argv[0]);
     exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(argv[1]));
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  FD_ZERO(&read_fds);
  FD_SET(s, &read_fds);
  
  root = argv[2];
  root_len = strlen(root);
    
  if ((bind(s,(struct sockaddr*) &server_addr, sizeof(struct sockaddr))) < 0) {
    perror("Bind: ");
    return;
  }
  if (listen(s,50000)<0) {
    perror("Listen : ");
    return;
  }
  if (server_addr.sin_addr.s_addr) {
    fprintf (stderr, "listening on address %d, port %d\n",
	     server_addr.sin_addr.s_addr, atoi(argv[1]));
  } else {
    fprintf (stderr, "listening on address (NULL), port %d\n",
	     atoi(argv[1]));
  }

}

int Reply (const Reply_in *in, Reply_out *, Reply_atoms *) {
  if (in->close) {
    closeSocket(in->socket);
  }
  else 
    returnSocket(in->socket);

  return 0;
}

#define BUFFER_SIZE 8192

int ReadRequest (const ReadRequest_in *in, ReadRequest_out *out, ReadRequest_atoms *) {
  char buf[BUFFER_SIZE];
  int rd;
  int length = 0; 
  int doneRequest = 0;
  out->request = 0;
  out->close = false;

  //DEBUG printf("ReadRequest in\n");
  do {
    rd = read(in->socket, buf+length, BUFFER_SIZE-length);
    if (rd == -1) {
      perror("Reading request");
      closeSocket(in->socket);
      return -1;
    } 
    else if (!rd) {
      usleep(1000*10);
    }
    else {
      char *start = buf;
      char *end;
      for (int i=0;i<rd;i++) {
	if (buf[i] == '\r') {
	  buf[i++] = 0;
	  buf[i] = 0; // Get rid of the \n
	  //DEBUG printf("%s\n", start);
	  if (length == 0) { // We're done...
	    doneRequest = true;
	    break;
	  }
	  else if (start[0] == 'G' && start[1] == 'E' && start[2] == 'T') {
	    start = strchr(start, ' ')+1;
	    end = strchr(start+1, ' ');
	    *end = 0;
	    while (*end != 'H')
	      end++;
	    while (*end != '/')
	      end++;
	    end++;
	    int major = *end-'0'; // HACK HACK HACK Assumes ASCII
	    end+=2;
	    int minor = *end-'0'; // HACK HACK HACK Assumes ASCII
	    if (major != 1 || (minor > 1))
	      printf("Urm, HTTP version: %d.%d we may be in trouble...\n", 
		     major, minor);
	    if (major == 1 && minor == 0) {
	      out->close = true;
	    }
	    
	    if (*start == '/')
	      start++;
	    out->request = strdup(start);
	  }
	  else if (start[0] == 'C' && start[1] == 'o' && start[2] == 'n' &&
		   strstr(start, "close")) {
	    out->close = true;
	  }
	  start=buf+i+1;
	  length = 0;
	}
	else {
	  length++;
	}
      }
      if (!doneRequest) {
	strncpy(buf, start, length);
      }
    }  
  } while (!doneRequest);

  if (!out->request) {
    out->request = "/sys_error.html";
  }

  out->socket = in->socket;
  // DEBUG printf("Request:%s:\n", out->request);
  return 0;
}

/*int Handler (Handler_in *in, Handler_out *out) {

}*/

int Page (const Page_in *in, Page_out *out, Page_atoms *) {

	return 0;
}

int ReadWrite (const ReadWrite_in *in, ReadWrite_out *out, ReadWrite_atoms *) {
  //FILE *f;
  int rd;
  struct stat sb;
  int res;
  char file_name[128];
	       
  out->socket = in->socket;
  out->close = in->close;
  sprintf(file_name, "%s/%s", root, in->file);
  //DEBUG printf("Looking for:%s:\n", file_name);
  res = stat(file_name, &sb);
  
  int file_size = sb.st_size;

  if (res < 0) {
    return 404;
  }
  else {
    int ix=0;
    //char *ptr;
    int header_len;
    if (suffixTest(in->file, ".html")) {
      out->content = "text/html";
    }
    else if (suffixTest(in->file, ".xhtml")) {
      out->content = "text/xhtml";
    }
    else if (suffixTest(in->file, ".svg")) {
      out->content = "image/svg+xml";
    }
    else if (suffixTest(in->file, ".png")) {
      out->content = "image/png";
    }
    else if (suffixTest(in->file, ".jpg") || suffixTest(in->file, ".jpeg")) {
      out->content = "image/jpeg";
    }
    else if (suffixTest(in->file, ".gif")) {
      out->content = "image/gif";
    }
    else {
      out->content = "text/plain";
    }
    //out->content="text/plain";
    char hdrs[8192];
    if (in->close) {
      sprintf(hdrs, "HTTP/1.1 200 OK\r\nContent-type: %s\r\nServer: Markov 0.1\r\nConnection: close\r\nContent-Length: %d\r\n\r\n", 
	      out->content, file_size);
    }
    else {
      sprintf(hdrs, "HTTP/1.1 200 OK\r\nContent-type: %s\r\nServer: Markov 0.1\r\nContent-Length: %d\r\n\r\n", 
	      out->content, file_size);
    }
    //printf(hdrs);
    header_len = strlen(hdrs);
    int written = write(in->socket, hdrs, header_len);
    while (written < header_len) {
      if (written < 0) {
	perror("Writing");
	closeSocket(in->socket);
	return -1;
      }
      written += write(in->socket, hdrs+written, header_len-written);
    }
    //out->length = sb.st_size+header_len;
    
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
      perror("Reading");
      closeSocket(in->socket);
      return -1;
    }
    //out->output = (char *)malloc(out->length);
    //memcpy(out->output, hdrs, header_len);
    //ptr = out->output+strlen(hdrs);
    char *pos;
    do {
      
      if ((rd = read(fd, hdrs, 8192)) < 0) {
	perror("read");
	closeSocket(in->socket);
	return 0;
      }
      ix += rd;
      pos = hdrs;
      while (rd > 0) {
	int ret = write(in->socket, hdrs, rd);
	if (ret < 0) {
	  perror("write");
	  closeSocket(in->socket);
	  close(fd);
	  return 0;
	}
	else if (ret == 0) {
	  rd = 0;
	  break;
	}
	else {
	  pos += ret;
	  rd -= ret;
	}
      }
    } while (ix < sb.st_size);
    close(fd);
  }
  //DEBUG printf("Done reading for:%s:\n", file_name);
  return 0;
}

//std::vector <Page_in *> *res = new std::vector<Page_in *>;
static std::vector <Listen_out *> *listen_outs = NULL;


int Listen (const Listen_in *, Listen_out *out, Listen_atoms *)
{
    if (listen_outs == NULL)
    {
        printf("creating new vector...\n");
        listen_outs = new std::vector<Listen_out *>;
    }

    int max; 
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 100*1000; // tenth of a second

    FD_ZERO(&read_fds);
    FD_SET(s, &read_fds);
    max = s;

    for (int i=0;i<1024;i++) 
    {
        if (socket_in_use[i]==0) 
        {
            if (i > max)
                max = i;
            FD_SET(i, &read_fds);
        }
    }

    int sel=0;
    int ix = 0;

    Listen_out *var;
    if (listen_outs->size() > 0)
    {
        int size = listen_outs->size();
        var = (Listen_out *)listen_outs->at(size-1);
        listen_outs->pop_back();
        out->socket = var->socket;
    }
    else
    {
        if ((sel=select(max+1, &read_fds, NULL, NULL, &select_timeout)) > 0)
        //if ((sel=select(max+1, &read_fds, NULL, NULL, NULL)) > 0)
        {
            int sock;
            if (FD_ISSET(s, &read_fds))
            {
                socklen_t length =  sizeof(struct sockaddr);
                sock = accept(s, (struct sockaddr *)&server_addr,&length);
                int optval = 1;
                if (setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof (optval)) < 0)
                {
                    perror("setsockopt");
                }
                var = new Listen_out();
                var->socket = sock;
                listen_outs->push_back(var);
                ix++;
                socket_in_use[sock] = 1;
            }
            for (int i=0;i<1024;i++)
            {
                if (socket_in_use[i] == 0)
                {
                    if (FD_ISSET(i, &read_fds))
                    {
                        var = new Listen_out();
                        var->socket = i;
                        listen_outs->push_back(var);
                        ix++;
                        FD_CLR(i, &read_fds);
                        socket_in_use[i] = 1;
                    }
                }
            }
            int size = listen_outs->size();
	    if(size <= 0) {
		return -1;
	    }
            var = (Listen_out *)listen_outs->at(size-1);
            listen_outs->pop_back();
            out->socket = var->socket;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

void writeHeaders(int socket_in, bool close, int length, const char *content) {
  char msg[128];
  
  sprintf(msg, "Content-Length: %d\r\n", length);
  write(socket_in, msg, strlen(msg));
  sprintf(msg, "Server: Markov 0.1\r\n");
  write(socket_in, msg, strlen(msg));
  sprintf(msg, "Content-Type: %s\r\n", content);
  write(socket_in, msg, strlen(msg));
  if (close)
    write(socket_in, "Connection: close\r\n",19); 
  write(socket_in, "\r\n", 2);
}

int FourOhF(const FourOhF_in *in, FourOhF_out *out, FourOhF_atoms *, int err) {
  const char *msg = "HTTP/1.1 404 File not found\r\n";
  write(in->socket, msg, strlen(msg));
  msg = "<html><body><h2>404 File Not Found!</h2></body></html>\n";
  writeHeaders(in->socket, in->close, strlen(msg), "text/html");
  write(in->socket, msg, strlen(msg));
  if (in->close)
    closeSocket(in->socket);
  else
    returnSocket(in->socket);

  return 0;
}

int SessionId(Page_in *in) {
  return in->socket;
}

int BadRequest(const BadRequest_in *in, BadRequest_out *, BadRequest_atoms *, int err) {
  const char *msg;

  switch (err) {

  case 400:
    msg = "HTTP/1.1 400 Bad Request\r\n";
    if (write(in->socket, msg, strlen(msg)) != -1) {
      msg = "<html><body><h2>400 Bad Request!</h2></body></html>\n";
      writeHeaders(in->socket, true, strlen(msg), "text/html");
        write(in->socket, msg, strlen(msg));
    }
    break;

  case 501:
    msg = "HTTP/1.1 501 Not Implemted\r\n";
    if (write(in->socket, msg, strlen(msg)) != -1) {
      msg = "<html><body><h2>501 Not Implemented!</h2></body></html>\n";
      writeHeaders(in->socket, true, strlen(msg), "text/html");
        write(in->socket, msg, strlen(msg));
    }
    break;

  case 408:
    msg = "HTTP/1.1 408 Request Timeout\r\n";
    if (write(in->socket, msg, strlen(msg)) != -1) {
      msg = "<html><body><h2>408 Request Timeout!</h2></body></html>\n";
      writeHeaders(in->socket, true, strlen(msg), "text/html");
        write(in->socket, msg, strlen(msg));
    }
    break;
  }

  closeSocket(in->socket);

  return 0;
}

