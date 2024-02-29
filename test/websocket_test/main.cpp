#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFFacilities.h"
#include "workflow/WFHttpServer.h"
#include "workflow/WFServer.h"
#include "workflow/WFWebSocketServer.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo) { wait_group.done(); }

void process_text(WebSocketChannel *ws, protocol::WebSocketFrame *in) {
    std::cout << "-----data len:" << in->get_parser()->payload_length << std::endl;

    ws->send_text((char *)in->get_parser()->payload_data, in->get_parser()->payload_length);
    ws->send_binary((char *)in->get_parser()->payload_data, in->get_parser()->payload_length);
    
    ws->send_frame(
        (char *)in->get_parser()->payload_data, 
        in->get_parser()->payload_length,
        in->get_parser()->payload_length,
        WebSocketFrameText);
}


int main(int argc, char *argv[]) {
    unsigned short port = 2333;
    // char *cert_file;
    // char *key_file;
    int ret;

    // if (argc != 2 && argc != 4) {
    //     fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
    //     fprintf(stderr, "ssl : %s <port> <cert_file> <key_file>\n", argv[0]);
    //     exit(1);
    // }

    // port = atoi(argv[1]);

    signal(SIGINT, sig_handler);

    WFWebSocketServer server;
    server.set_ping_interval(20*1000);
    

    // if (argc == 4) {
    //     cert_file = argv[2];
    //     key_file = argv[3];

    //     ret = server.start(port, cert_file, key_file);
    // } else {
        ret = server.start(0);
    // }

   /*  struct sockaddr *addr;
    socklen_t *addrlen;

    int res = server.get_listen_addr(addr, addrlen);

    if (addr->sa_family == AF_INET) { // IPv4地址  
        sockaddr_in* ipv4 = (sockaddr_in*)addr;  
        std::cout << "IPv4 Address: " << inet_ntoa(ipv4->sin_addr) << std::endl;  
    } else if (addr->sa_family == AF_INET6) { // IPv6地址  
        sockaddr_in6* ipv6 = (sockaddr_in6*)addr;  
        char ipStr[INET6_ADDRSTRLEN];  
        inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipStr, INET6_ADDRSTRLEN);  
        std::cout << "IPv6 Address: " << ipStr << std::endl;  
    } else {  
        std::cout << "Unsupported address family" << std::endl;  
    }   */

    server.set_process_text_fn(process_text);

    if (ret == 0) {
        wait_group.wait();
        server.stop();
    } else {
        perror("Cannot start server");
        exit(1);
    }

    return 0;
}