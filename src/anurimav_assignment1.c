/**
 * @anurimav_assignment1
 * @author  Anurima Vaishnavi Kumar <anurimav@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <errno.h>
#include "../include/global.h"
#include "../include/logger.h"

#define input_ 0 // file descriptor for standard input_
#define MAXDATASIZE 500 // max number of bytes we can get at once
#define BACKLOG 10 //listener's queue length
#define TRUE 1 
#define CMD_SIZE 100 // size of the command
#define BUFFER_SIZE 256 //buffer size for the receiver function
#define MAXX 60000

struct host{
    char port_num[MAXDATASIZE];
    char ip[MAXDATASIZE];
    bool logged_in;
    char hostname[MAXDATASIZE];
    int is_server;
    int file_desc;
    struct message * queue;
    struct host * blocked;
    struct host * next;
    int sent;
    int rcv;
};

struct message
{
    char msg[MAXX];
    bool broadcast;
    struct message * next_message;
    struct host * sender;
};

// localhost is for initializing the host
struct host * localhost = NULL;
//nclient = newclient whenever a new incoming connection comes to the server
struct host * nclient = NULL;
// there is one server
struct host * server = NULL;
//there are multiple clients
struct host * clients = NULL;



void populate_host(char * host,char * port);
void execute(char command[],int fd);
void server_initialisation();
void client_initialisation();
void server_specific_commands(char command[], int fd);
void client_specific_commands(char command[], int fd);
void login__client(char ip[], char port[]);
int client_server_connect(char ip[], char port[]);
int isValidIpAddress(char command[MAXDATASIZE]);
void exit_client();
void server_send(char receiver_ip[],int fd,char msg[],int broadcast);
void server_client_exit(int fd);
void client_send_message(char command[]);
void logged_in_clients_list(char command[]);
void server_refresh_client(int fd);
void server_client_login(char ip[], char port[], char hostname[], int fd);
void block_unblock_client(char command[],char ip[],int value);
void block_unblock_server(char command[],int fd,int value);
void server_blocked(char ip[]);

void client_initialisation()
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *new, *info;
    int rv, status;
    char s[INET6_ADDRSTRLEN];
    int server_socket = 0;
    int yes=1; //  for setsockopt() SO_REUSEADDR, below
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_family = AF_UNSPEC; //don't care IPv4 or IPv6
    /* Fill up address structures */
    if ((getaddrinfo(NULL, localhost -> port_num, &hints, &info) != 0)) {
        exit(EXIT_FAILURE);
    }
    
    new = info;
    while(new != NULL){
        /* Socket */
        if ((server_socket = socket(new->ai_family, new->ai_socktype, new->ai_protocol)) == -1) {
            new= new->ai_next;
            continue;
        }
        // lose the pesky "address already in use" error message
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
              exit(EXIT_FAILURE);
        }
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1) {
              exit(EXIT_FAILURE);
        }
        if (bind(server_socket, new->ai_addr, new->ai_addrlen) == -1) {
            close(server_socket);
            new = new->ai_next;
            continue;
        }
        break;
    }

     // exit if could not bind
    if (new==NULL)
   {
        exit(EXIT_FAILURE);
    }
    //listen
    if (listen(server_socket, BACKLOG) == -1) {
        exit(EXIT_FAILURE);
    }
    localhost -> file_desc = server_socket;
    freeaddrinfo(new); // free the linked list
    // for client we only care about STDIN
    for(;;)
    {
        //streaming input_ to a socket
        char * command = (char * ) malloc(sizeof(char) * MAXX);
        memset(command, '\0', MAXX);
        if (fgets(command, MAXX - 1, stdin) == NULL) {} 
        else {
            execute(command, input_);
            }
        fflush(stdout);

    }

}

//Check if the IPV4 address is valid. Note that the below function isn't IPV6 compatible
int isValidIpAddress(char command[MAXDATASIZE])
{
    int len = strlen(command);
    if (len < 7 || len > 15)
        return 0;
    struct sockaddr_in sa;
    int ans = inet_pton(AF_INET, command, & (sa.sin_addr));
    if (ans==0)
    {
        return 0;
    }
    else{
        return 1;
    }
}

void execute(char command[], int fd)
{
       if(strstr(command,"PORT") !=NULL)
    {
        cse4589_print_and_log("[PORT:SUCCESS]\n");
        cse4589_print_and_log("PORT:%s\n",localhost -> port_num);
        cse4589_print_and_log("[PORT:END]\n");
    }
    if(strstr(command,"AUTHOR") !=NULL)
    {
        cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
        cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "anurimav");
        cse4589_print_and_log("[AUTHOR:END]\n");
    }
    if(strstr(command,"IP") !=NULL)
    {
        cse4589_print_and_log("[IP:SUCCESS]\n");
        cse4589_print_and_log("IP:%s\n", localhost -> ip);
        cse4589_print_and_log("[IP:END]\n");
    }

    if (strcmp(command,"LIST\n") ==0) {
        int exe = 0;
        if(localhost->is_server==0)
        {

            if(!localhost -> logged_in)
            {
                cse4589_print_and_log("[LIST:ERROR]\n");
                cse4589_print_and_log("[LIST:END]\n");
                exe=1;
            }
        }
        if (exe==0)
        {
        cse4589_print_and_log("[LIST:SUCCESS]\n");
        int idx = 0;
        struct host * head = clients;
        while(head!=NULL)
        
        {
            if (head -> logged_in){
            idx+=1;
            cse4589_print_and_log("%-5d%-35s%-20s%-8s\n", idx, (head -> hostname), (head->ip), (head->port_num));}
            head = head -> next;
        }

        cse4589_print_and_log("[LIST:END]\n");
    }
    }
    
    if(localhost->is_server==0)
    {
    client_specific_commands(command,fd);
    }
    else
    {
      server_specific_commands(command,fd);
    }
      fflush(stdout);
    
}

void server_specific_commands(char command[], int fd)
{

    // //checking if the command has login in it
    if (strstr(command, "LOGIN")!=NULL)
    {
        char ip[MAXDATASIZE];
        char port[MAXDATASIZE];
        char hostname[MAXDATASIZE];
        // sprintf(var, "LOGIN %s %s %s\n", localhost -> ip, localhost -> port_num,localhost -> hostname );
        // the client sent the arguments in the order : ip, port_num, hostname
        sscanf(command, "LOGIN %s %s %s", ip, port, hostname);
        server_client_login(ip,port,hostname,fd);
    }
    else if (strstr(command, "STATISTICS") != NULL)
    {
        cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
        struct host * temp = clients;
        int idx = 1;
        char status[MAXX];
        while(temp)
        {
            if(temp->logged_in)
            {
                strcpy(status,"logged-in");
            }
            else
            {
                strcpy(status,"logged-out");
            }
            cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", idx, temp -> hostname ,  temp -> sent, temp->rcv,status);
            temp = temp->next;
            idx+=1;
        }
        cse4589_print_and_log("[STATISTICS:END]\n");
    }
    else if (strstr(command, "EXIT") != NULL) {
        server_client_exit(fd);
    }
    else if (strstr(command, "REFRESH") != NULL) {
       server_refresh_client(fd);
    }
    else if(strstr(command, "BLOCKED") != NULL)
    {
        char ip[MAXDATASIZE];
        sscanf(command, "BLOCKED %s\n", ip);
        server_blocked(ip);
    }
    else if (strstr(command, "BLOCK") != NULL){
        if(strstr(command,"UNBLOCK") != NULL)
        {
            block_unblock_server(command,fd,0);
        }
        else
        {
            block_unblock_server(command,fd,1);
        }
    }
    else if (strstr(command,"SEND") != NULL)
    {
        char ip[MAXX], msg[MAXX];
        int idx=0, a=5;
        int val;
        while(command[a] != ' ')
        {
            ip[idx] = command[a];
            idx+=1;
            a+=1;
        }
        a+=1;
        ip[idx] = '\0';
        idx = 0;
        while(command[a] != '\0')
        {
            msg[idx] = command[a];
            idx+=1;
            a+=1;
        }
        msg[idx-1] = '\0'; 
        int length = strlen(msg);
        if(length>256)
        {
            val = send(fd,"SENDERROR\n",strlen("SENDERROR\n")+1,0);
            return;
        }
        for (int i = 0; i < length; i++) {
        if (msg[i] < 0 || msg[i] > 127) {
            val = send(fd,"SENDERROR\n",strlen("SENDERROR\n")+1,0);
            return;
        }
        }
        server_send(ip,fd,msg,0);
    }

    else if(strstr(command,"LOGOUTCLIENT") != NULL)
    {
        char ip[MAXX];
        sscanf(command, "LOGOUT %s\n", ip);
        struct host * temp = clients;
        while(temp)
        {
            if(temp->file_desc == fd)
            {

                temp -> logged_in = false;
                break;
            }
          temp = temp -> next;
        }
        int val;
        if(temp == NULL)
        {
            val = send(temp->file_desc,"LOGOUTERROR\n",strlen("LOGOUTERROR\n")+1,0);
        }
        else
        {
            val = send(temp->file_desc,"LOGOUTSUCCESS\n",strlen("LOGOUTSUCCESS\n")+1,0);
        }
        if(val == -1){}
        return;
    }

    else if (strstr(command, "BROADCAST") != NULL)
    {
        char message[MAXX];
        char from_ip[MAXX];
        sscanf(command, "BROADCAST %s\n", message);
        struct host * temp = clients;
        while(temp)
        {
            if(temp->file_desc == fd)
            {
                break;
            }
            temp = temp-> next;
        }
        strcpy(from_ip ,temp->ip);
        temp = clients;
        while(temp)
        {
            if(temp->file_desc != fd)
            {
             server_send(temp->ip,fd,message,1);
            }
            temp = temp -> next;
        }
        int val = send(fd,"BROADCASTSUCCESS\n",strlen("BROADCASTSUCCESS\n")+1,0);
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_ip , "255.255.255.255", message);
        cse4589_print_and_log("[RELAYED:END]\n");
        return;
    }
}
    

void client_specific_commands(char command[], int fd){
    // Login sends two other arguments <server ip> <server port>
    if (strstr(command, "LOGIN")!=NULL)
    {
        char ip[MAXDATASIZE],port[MAXDATASIZE];
        int a=0, b=0, str=6;
        while(a<=255 && command[str] !=' ')
        {
            ip[a] = command[str];
            a+=1;
            str+=1;
        }
        ip[a] = '\0';
        a+=1;
        while(command[str] !='\0'){
            port[b] = command[str];
            b+=1;
            str+=1;
        }
        port[b-1] ='\0';
        login__client(ip, port);

    }
    else if (strcmp(command, "EXIT\n") == 0) {
        exit_client();
    }
    else if (strcmp(command, "REFRESH\n") == 0) {
        if (localhost -> logged_in) {
            int s;
            s = send(server->file_desc,command,strlen(command)+1,0);
            if(s==-1){
            }
        } else {
            cse4589_print_and_log("[REFRESH:ERROR]\n");
            cse4589_print_and_log("[REFRESH:END]\n");
        }
    }
    else if(strstr(command,"SUCCESSSEND") != NULL)
    {
        cse4589_print_and_log("[SEND:SUCCESS]\n");
        cse4589_print_and_log("[SEND:END]\n");
        return;
    }

    else if(strstr(command, "SEND") != NULL)
    {
        if(localhost->logged_in == false)
        {
            cse4589_print_and_log("[SEND:ERROR]\n");
            cse4589_print_and_log("[SEND:END]\n");
            return;
        }
        else{
            struct host * new = clients;
            char ip[MAXDATASIZE];
            int a=0, str=5;
            while(a<=255 && command[str] !=' ')
            {
                ip[a] = command[str];
                a+=1;
                str+=1;
            }
            ip[a] = '\0';
            if(isValidIpAddress(ip)==0)
            {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                return;
            }
            while(new!=NULL)
            {
                if(strcmp(ip,new->ip) != 0)
                {
                    new = new->next;
                }
                else
                {
                    break;
                }
            }
            if(new==NULL){
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                return;
            }

           struct host * temp = clients;
            while(temp != NULL)
            { 
                if(strcmp(localhost->ip,temp->ip)==0)
                {
                  break;
                }
               temp = temp->next;
                   
            }
            int value = send(server->file_desc, command, strlen(command) + 1, 0);
            if(value == -1){}
        }
    }

    else if (strstr(command,"BROADCASTSUCCESS") != NULL)
    {
        cse4589_print_and_log("[BROADCAST:SUCCESS]\n");
        cse4589_print_and_log("[BROADCAST:END]\n");
        return;
    }

    else if (strstr(command, "BROADCAST") != NULL)
    {
        if(localhost->logged_in == false)
        {
            cse4589_print_and_log("[BROADCAST:ERROR]\n");
            cse4589_print_and_log("[BROADCAST:END]\n");
            return;
        }
        else
        {
            int val = send(server -> file_desc, command, strlen(command) + 1, 0);
            if(val == -1){}
        }
    }

    else if((strcmp(command,"LOGOUTSUCCESS\n") == 0))
    {
        cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
        cse4589_print_and_log("[LOGOUT:END]\n");
        localhost -> logged_in = false;
        return;
    }

    else if((strcmp(command,"LOGOUTERROR\n") ==0 ))
    {
        cse4589_print_and_log("[LOGOUT:ERROR]\n");
        cse4589_print_and_log("[LOGOUT:END]\n");
        return;
    }

    else if(strcmp(command,"LOGOUT\n") == 0)
    {
        if(localhost->logged_in == false)
        {
            cse4589_print_and_log("[LOGOUT:ERROR]\n");
            cse4589_print_and_log("[LOGOUT:END]\n");
            return;
        }
        char ans[60000];
        sprintf(ans, "LOGOUTCLIENT %s\n",localhost-> ip);
        int val = send(server->file_desc,ans,strlen(ans)+1,0);
        if(val== -1){}
    }

    else if(strstr(command,"QUEUEMESSAGES") != NULL)
    {
        char * token = strtok(command,"\n");
        char sender[MAXX], msg[MAXX];
        while(token!= NULL)
    {
        char sender[MAXX], msg[MAXX];
        token = strtok(NULL, "\n");
        if (strstr(token, "END") != NULL) { continue;}
        if (strstr(token, "CLIENTLIST") != NULL){
            char response[100000] = "CLIENTLIST\n";
            token = strtok(NULL, "\n");
            while(token!=NULL)
            {
             if (strstr(token, "END") != NULL) { break;}   
                char var[500 * 5];
                char ip[MAXDATASIZE], port[MAXDATASIZE], host[MAXDATASIZE];
                sscanf(token, "%s %s %s\n", ip, port,host);
                sprintf(var, "%s %s %s\n", ip,port, host );
                strcat(response, var);
                token = strtok(NULL, "\n");
            }
            strcat(response, "END\n");
            logged_in_clients_list(response);
            return;
        }
        sscanf(token, "%s %s\n", sender, msg);
        cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s\n[msg]:%s\n",sender, msg);
        cse4589_print_and_log("[RECEIVED:END]\n");
    }
    }
    else if(strstr(command,"RECEIVE") != NULL)
    {
        int idx = 0;
        char ip[MAXX];
        char msg[MAXX];
        int a = 8;
        while(command[a] != ' ' && idx<=255)
        {
            ip[idx] = command[a];
            a+=1; idx+=1;
        }
        a+=1;
        ip[idx] = '\0';
        idx = 0;
        while(command[a] != '\0')
        {
            msg[idx] = command[a];
            a+=1; idx+=1;
        }
        msg[idx-1] = '\0';
         cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
         cse4589_print_and_log("msg from:%s\n[msg]:%s\n", ip, msg);
         cse4589_print_and_log("[RECEIVED:END]\n");
 
    }

    else if (strstr(command, "CLIENTLIST") != NULL) {
        logged_in_clients_list(command);
    }

    else if (strcmp(command, "UNBLOCKERROR\n") == 0)
    {
        cse4589_print_and_log("[UNBLOCK:ERROR]\n");
        cse4589_print_and_log("[UNBLOCK:END]\n");
    }

    else if (strcmp(command, "BLOCKERROR\n") == 0)
    {
        cse4589_print_and_log("[BLOCK:ERROR]\n");
        cse4589_print_and_log("[BLOCK:END]\n");
    }

    else if (strcmp(command, "UNBLOCKSUCCESS\n") == 0)
    {
        cse4589_print_and_log("[UNBLOCK:SUCCESS]\n");
        cse4589_print_and_log("[UNBLOCK:END]\n");
    }

    else if (strcmp(command, "BLOCKSUCCESS\n") == 0)
    {
        cse4589_print_and_log("[BLOCK:SUCCESS]\n");
        cse4589_print_and_log("[BLOCK:END]\n");
    }

    else if (strstr(command, "BLOCK") != NULL){
        if(localhost-> logged_in == false)
        {
            if(strstr(command,"UNBLOCK") != NULL)
            {
                cse4589_print_and_log("[UNBLOCK:ERROR]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
            }
            else
            {

                cse4589_print_and_log("[BLOCK:ERROR]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
            }
        }
        else
        {
            int flag;
            char ip[MAXDATASIZE];
            int a=0,str;
            if(strstr(command, "UNBLOCK") != NULL)
            {
                flag = 0;
                str = 8;

            }
            else
            {
                flag = 1;
                str = 6;
            }
            while(a<=255 && command[str] != '\0')
            {
               ip[a] = command[str];
               a+=1;
               str+=1;
            }
            ip[a] = '\0';
        block_unblock_client(command,ip,flag);
        }    
    }

}



void client_send_message(char command[])
{
    char ip[MAXX];
    int idx = 0;
    int a = 5;
    char msg[MAXX];
    while(command[a] != ' ')
    {
        ip[idx] = command[a];
        idx+=1;
        a+=1;
    }
    ip[idx] = '\0';
    // check if it's a valid IP address
    int yes = isValidIpAddress(ip);
    if (yes==0)
    {
        cse4589_print_and_log("[SEND:ERROR]\n");
        cse4589_print_and_log("[SEND:END]\n");
        return;
    }
    // check if the client ip is existing in the list of clients
    struct host * client = clients;
    for(;;)
    {
        if(client == NULL)
        {
            break;
        }
        else
        {
            if(strstr(client->ip,ip)!=NULL)
            {
                break;
            }
        }
        client = client -> next;
    }
    if(client!=NULL)
    {
        // check if the client is in the blocked list of clients
        struct host * blocked_list = client ->blocked;
        while(blocked_list)
        {
            if(strstr(blocked_list->ip,ip) != NULL)
            {
                cse4589_print_and_log("[SEND:ERROR]\n");
                cse4589_print_and_log("[SEND:END]\n");
                return;
            }
            blocked_list = blocked_list->next;
        }
        int value = send(server->file_desc, command, strlen(command) + 1, 0);
        if(value == -1){}
    }
    else
    {
        cse4589_print_and_log("[SEND:ERROR]\n");
        cse4589_print_and_log("[SEND:END]\n");
    }

}

void block_unblock_client(char command[], char ip[],int value)
{
    int a;
    if(value == 0)
    {
        sscanf(command, "UNBLOCK %s\n", ip);
    }
    else
    {
        sscanf(command, "BLOCK %s\n", ip);
    }
    // char tempcommand[1024];
    // =strtok()
    if(ip==NULL)
    {
        if(value == 1)
        {
            cse4589_print_and_log("[BLOCK:ERROR]\n");
            cse4589_print_and_log("[BLOCK:END]\n");
            return;
        }
        else
        {
            cse4589_print_and_log("[UNBLOCK:ERROR]\n");
            cse4589_print_and_log("[UNBLOCK:END]\n");
            return;
        }
    }
    int b = isValidIpAddress(ip);
    if(b==0)
    {
        if(value==1)
        {
            cse4589_print_and_log("[BLOCK:ERROR]\n");
            cse4589_print_and_log("[BLOCK:END]\n");
            return;
        }
        else
        {
            cse4589_print_and_log("[UNBLOCK:ERROR]\n");
            cse4589_print_and_log("[UNBLOCK:END]\n");
            return;
        }
    }
    // check if the ip is there in the list of currently logged in clients
    struct host * temp = clients;
    int found = 0;
    while(temp!=NULL)
    {
        if(strcmp(temp -> ip, ip) == 0)
        {
            found = 1;
            break;
        }
        temp = temp->next;
    }
    if(found == 0)
    {
        if(value==1)
        {
            
            cse4589_print_and_log("[BLOCK:ERROR]\n");
            cse4589_print_and_log("[BLOCK:END]\n");
            return;
        }
        else
        {
            cse4589_print_and_log("[UNBLOCK:ERROR]\n");
            cse4589_print_and_log("[UNBLOCK:END]\n");
            return;
        }
    }
    // if you found the client in the list of the logged in list. 
    struct host * block_unblock = temp;
    // check if the client is already blocked/unblocked
    temp = localhost -> blocked;
    struct host * prev = NULL;
    while(temp!=NULL)
    {
        if(strstr(temp->ip,block_unblock->ip)!=NULL)
        {
            if(value==1)
          {
            cse4589_print_and_log("[BLOCK:ERROR]\n");
            cse4589_print_and_log("[BLOCK:END]\n");
            return;
          }
          else
          {
            if(prev){prev->next = temp->next;}
            else{localhost -> blocked = NULL;}
            int s = send(server->file_desc, command, strlen(command) + 1, 0);
            if(s == -1)
            {
                // print something
            }
            return;
          }

        }
        prev = temp;
        temp = temp->next;
    }
    if(temp ==NULL)
    {
        if(value==1)
        {
            struct host * block_client = malloc(sizeof(struct host));
            block_client -> file_desc = block_unblock ->file_desc;
            block_client -> next = NULL;
            memcpy(block_client -> port_num, block_unblock -> port_num, sizeof(block_client -> port_num));
            memcpy(block_client -> hostname, block_unblock -> hostname, sizeof(block_client -> hostname));
            memcpy(block_client -> ip, block_unblock -> ip, sizeof(block_client -> ip));
            if(prev == NULL)
            {
                localhost->blocked = block_client;
            }
            else
            {
                prev -> next = block_client;
            }
            int s = send(server->file_desc, command, strlen(command) + 1, 0);
            if(s == -1)
            {
                // print something
            }
            return;
        }
        else
        {
            cse4589_print_and_log("[UNBLOCK:ERROR]\n");
            cse4589_print_and_log("[UNBLOCK:END]\n");
            return;
        }
    }
}

void block_unblock_server(char command[],int fd, int value)
{
    char ip[MAXDATASIZE];
    if(value == 0)
    {
        sscanf(command, "UNBLOCK %s\n", ip);
    }
    else
    {
        sscanf(command, "BLOCK %s\n", ip);
    }
    struct host * temp = clients;
    struct host * to = malloc(sizeof(struct host));
    struct host * from = malloc(sizeof(struct host));
    while(temp!=NULL)
    {
        if(strstr(temp->ip, ip) != NULL)
        {
            to = temp;
        }

        if(temp->file_desc == fd)
        {
            from = temp;
        }
        temp = temp -> next;
    }

    if(to == NULL)
    {
        if (value == 0){
        int s = send(from->file_desc, "UNBLOCKERROR\n", strlen("UNBLOCKERROR\n") + 1, 0);
        if(s == -1)
        {}
        return;
        }
        else
        {
        int s = send(from->file_desc, "BLOCKERROR\n", strlen("BLOCKERROR\n") + 1, 0);
        if(s == -1)
        {}
        return;
        }
    }
    else
    {
        if(value == 0)
        {
            struct host * temp = from->blocked;
            struct host * prev = NULL;
            while(temp != NULL)
            {
                if(strstr(temp->ip, to->ip) != NULL)
                {
                    if(prev == NULL)
                    {
                        from -> blocked = prev;
                    }
                    else
                    {
                        prev -> next = temp -> next;
                    }
                }
                prev = temp;
                temp = temp -> next;
            }
            int s = send(from->file_desc, "UNBLOCKSUCCESS\n", strlen("UNBLOCKSUCCESS\n") + 1, 0);
            if(s == -1){}
            return;
        }
        else
        {

            struct host * block_client = malloc(sizeof(struct host));
            block_client -> file_desc = to ->file_desc;
            block_client -> next = NULL;
            memcpy(block_client -> port_num, to -> port_num, sizeof(block_client -> port_num));
            int port = block_client -> port_num;
            memcpy(block_client -> hostname, to -> hostname, sizeof(block_client -> hostname));
            memcpy(block_client -> ip, to -> ip, sizeof(block_client -> ip));
            struct host * temp = from -> blocked;
            if(temp == NULL)
            {
               from -> blocked = malloc(sizeof(struct host));
               from -> blocked = block_client;
            }
            else if (atoi(port)<atoi(temp->port_num))
            {
                block_client -> next = temp;
                from -> blocked = block_client;
            }
            else
           {
            struct host * temp = from -> blocked;
            struct host * prev = NULL;
            while(temp!=NULL)
            {
                if (atoi(port)<atoi(temp->port_num))
                {
                    if(prev != NULL)
                    {
                        prev -> next = block_client;
                        block_client->next = temp;
                        break;
                    }
                    else
                    {
                        block_client -> next = temp;
                        from -> blocked = block_client;
                    }
                }
                prev = temp;
                temp = temp -> next; 
                    
            }
            if(temp == NULL)
            {
                prev->next = block_client;
            }
}
         
        int s = send(from->file_desc, "BLOCKSUCCESS\n", strlen("BLOCKSUCCESS\n") + 1, 0);
        if(s == -1){} return;
        }
    }
}


void server_blocked(char ip[])
{
    if (ip == NULL)
    {
        cse4589_print_and_log("[BLOCKED:ERROR]\n");
        cse4589_print_and_log("[BLOCKED:END]\n");
        return;
    }
    int a = isValidIpAddress(ip);
    struct host * temp = clients;
    struct host * block_list_client = NULL;
    if(a==0)
    {
        cse4589_print_and_log("[BLOCKED:ERROR]\n");
        cse4589_print_and_log("[BLOCKED:END]\n");
        return;
    }
    else
    {
        while(temp!=NULL)
        {
            if(strstr(temp->ip, ip)!=NULL)
            {
                block_list_client = temp;
                break;
            }
            temp= temp->next;
        }
        if (temp == NULL)
        {
            cse4589_print_and_log("[BLOCKED:ERROR]\n");
            cse4589_print_and_log("[BLOCKED:END]\n");
            return;
        }
    }
    struct host * list = block_list_client-> blocked;
    int id = 0;
    cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
    while(list!=NULL)
    {
        id+=1;
        cse4589_print_and_log("%-5d%-35s%-20s%-8s\n", id, list -> hostname,  list->ip, list->port_num);
        list = list -> next;
    }
    cse4589_print_and_log("[BLOCKED:END]\n");
    return;
}

void server_send(char receiver_ip[],int fd,char msg[], int broadcast)
{
    struct host * from = malloc(sizeof(struct host));
    struct host * to = malloc(sizeof(struct host));
    struct host * client = clients;
    while(client != NULL)
    {
        if (fd == client->file_desc)
        {
            from = client;
        }
        if (strstr(receiver_ip, client->ip) != NULL)
        {
            to = client;
        }
        client = client -> next;
    }
    if(from == NULL || to == NULL)
    {
        if(broadcast == 0){
        cse4589_print_and_log("[RELAYED:ERROR]\n");
        cse4589_print_and_log("[RELAYED:END]\n");
        }
        return;
    }
    struct host * blocked_list = from->blocked;
    while(blocked_list)
    {
        if(strstr(blocked_list->ip,receiver_ip) != NULL)
            {
            if(broadcast == 0){
            cse4589_print_and_log("[RELAYED:ERROR]\n");
            cse4589_print_and_log("[RELAYED:END]\n");
            }
            return;
        }
            blocked_list = blocked_list->next;
    }
    bool block = false;
    struct host * temp = malloc(sizeof(struct host));
    from -> sent += 1;
    temp = to ->blocked;
    while(temp)
    {
        if (strstr(from->ip,temp->ip) != NULL)
        {
            block = true;
            break;
        }
        temp = temp -> next;
    }
    char var[MAXX];
    int val;
    char receive[MAXX];
    if(broadcast == 1)
    {
        
    }
    else{
        val = send(from->file_desc,"SUCCESSSEND\n",strlen("SUCCESSSEND\n")+1,0);
    }
    if(block)
    {
        if(broadcast == 0){
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from -> ip, to -> ip, msg);
        cse4589_print_and_log("[RELAYED:END]\n");
        }
         return;
    }
    if(to ->logged_in)
    {
        sprintf(receive, "RECEIVE %s %s\n", from -> ip, msg);
        val = send(to->file_desc,receive,strlen(receive)+1,0);
        to->rcv += 1;
        if(broadcast == 0){
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from -> ip, to -> ip, msg);
        cse4589_print_and_log("[RELAYED:END]\n");
        }
    }
    else
    {
        struct message * new = malloc(sizeof(struct message));
        struct message * messages = to->queue;
        memcpy(new->msg,msg,sizeof(new->msg));
        if(broadcast==0)
        {new -> broadcast = false;}
        else
        {new -> broadcast = true;}
        new -> sender = from;
        if(to->queue != NULL)
        {
            struct message * prev = NULL;
            while(messages!=NULL)
            {
                prev = messages;
                messages = messages ->next_message;
            }
            prev-> next_message = new;
        }
        else
        {
            to ->queue = new;
        }
    }
}

void server_refresh_client(int fd)
{
    struct host * node = clients;
    char res[100000] = "CLIENTLIST REFRESH\n";
    int val;
    while(node!=NULL)
    {
        if (node->logged_in == true)
        {
            char ans[60000];
            sprintf(ans, "%s %s %s\n", node -> ip,  node-> port_num, node -> hostname);
            strcat(res, ans);
        }
        node = node -> next;
    }
    strcat(res, "END\n");
    val =  send(fd, res, strlen(res) + 1, 0);
    if (val==-1)
    {
        //error
    }
 }

void logged_in_clients_list(char command[])
{
    clients = malloc(sizeof(struct host));
    int refresh = 0;
    if (strstr(command,"REFRESH") != NULL)
    {
        refresh = 1;
    }
    struct host * temp = clients;
    // returns the first token
    char * token = strtok(command,"\n");
    while(token!= NULL)
    {
        char ip[MAXX] , port[MAXX] , hostname[MAXX];
        token = strtok(NULL, "\n");
        if (strstr(token, "END") != NULL) {break;}
        sscanf(token, "%s %s %s\n", ip, port, hostname);
        struct host * nclient = malloc(sizeof(struct host));
        nclient -> logged_in = true;
        temp -> next = nclient;
        temp = temp->next;
        memcpy(nclient-> ip, ip, sizeof(nclient-> ip));
        memcpy(nclient -> hostname, hostname, sizeof(nclient -> hostname));
        memcpy(nclient -> port_num, port, sizeof(nclient -> port_num));
    }
    clients = clients->next;
    if(refresh==1)
    {
        cse4589_print_and_log("[REFRESH:SUCCESS]\n");
        cse4589_print_and_log("[REFRESH:END]\n");
    } else {
        cse4589_print_and_log("[LOGIN:SUCCESS]\n");
        cse4589_print_and_log("[LOGIN:END]\n");
    }

}



// Logout from the server (if logged-in) and terminate the application with exit code 0. 
// This should delete all the states for this client on the server. You can assume that an EXITed client will never start again.

// send a message to the server that you are exiting. 
void exit_client()
{
    int s = send(server ->file_desc,"EXIT",strlen("EXIT")+1, 0);
    if(s != -1)
    {
        cse4589_print_and_log("[EXIT:SUCCESS]\n");
        cse4589_print_and_log("[EXIT:END]\n");
        exit(0);
    }
}

void server_client_exit(int fd)
{

    if (clients->file_desc == fd)
    {
        clients = clients->next;
    }
    else{
        struct host * prev = clients;
        struct host * node = clients->next;
        while(node!=NULL)
        {
            if(node->file_desc == fd)
            {
                prev->next = node->next;
                break;
            }
            node = node -> next;
        }

    }
    return;
}



void populate_host(char * host,char * port)
{
    //Initializing the localhost here based on whether the second argument is c or s
    localhost = malloc(sizeof(struct host));
    // Assigning the values to the host name and the IP address of the host
    char *IPbuffer, hostbuffer[MAXDATASIZE];
    int hostname;
    struct hostent * host_entry;
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    host_entry = gethostbyname(hostbuffer);
    memcpy(localhost -> ip, inet_ntoa( * ((struct in_addr * ) host_entry -> h_addr_list[0])), sizeof(localhost-> ip));
    memcpy(localhost -> hostname, hostbuffer, sizeof(localhost -> hostname));
    if (strcmp(host,"c")==0){
        localhost -> is_server = 0;
    }
    else{
        localhost -> is_server = 1;
    }
    memcpy(localhost -> port_num, port, sizeof(localhost -> port_num));
    if (strcmp(host,"c")==0)
    {
        client_initialisation();

    }
    else{
        server_initialisation();
    }
}

// get sockaddr, IPv4 or IPv6:
 void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
     return &(((struct sockaddr_in*)sa)->sin_addr);
    }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//Client will connect to the server using the connect()
// we will create a listening socket to receive messages sent from another clients via server

void login__client(char ip[], char port[])
{
    int length = MAXDATASIZE * 3;
    char message[length];
    int val=0;
    char buf[100000]; // buffer for server data
    int nbytes;
    if(port == NULL || ip == NULL)
    {
        cse4589_print_and_log("[LOGIN:ERROR]\n");
        cse4589_print_and_log("[LOGIN:END]\n");
        return;
    }
    // server will be equal to NULL when the client trying to the connect to the server for the first time
    if(server == NULL)
    {
        int a = client_server_connect(ip, port);
        int b = isValidIpAddress(ip);
        if (a==0)
        {
            cse4589_print_and_log("[LOGIN:ERROR]\n");
            cse4589_print_and_log("[LOGIN:END]\n");
            return;
        }
        if (b==0)
        {
            cse4589_print_and_log("[LOGIN:ERROR]\n");
            cse4589_print_and_log("[LOGIN:END]\n");
            return;
        }
    }
    else{
        if (strcmp(server->ip,ip)!=0)
        {
            cse4589_print_and_log("[LOGIN:ERROR]\n");
            cse4589_print_and_log("[LOGIN:END]\n");
            return;
        }
        if(strcmp(server->port_num,port)!=0)
        {
            cse4589_print_and_log("[LOGIN:ERROR]\n");
            cse4589_print_and_log("[LOGIN:END]\n");
            return;
        }
    }
    char var[MAXDATASIZE*100];
    //sprintf stores the message in the buffer instead of printing to the console
    sprintf(var, "LOGIN %s %s %s\n", localhost -> ip, localhost -> port_num,localhost -> hostname );
    localhost -> logged_in = true;
    val = send(server->file_desc,var,strlen(var)+1,0);
    // here you are sending the message to the server so that it can update the next pointer of the clients. 
    if(val==-1)
    {
        exit(-1);
    }
    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    int fdmax; // maximum file descriptor number
    int fd=0; // for looping all the file descriptors present in the read_fds
    int newfd;
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    struct sockaddr_storage peer; // peer_client address
    socklen_t addrlen;
    if (server->file_desc > localhost->file_desc)
    {
        fdmax = server->file_desc;
    }
    else
    {
         fdmax = localhost->file_desc;
    }

    if (fdmax < input_)
    {
        fdmax = input_;
    }
    FD_SET(localhost->file_desc,&master);
    FD_SET(server->file_desc,&master);
    FD_SET(input_ ,&master);
    bool is_logged_in = localhost -> logged_in;
    while(is_logged_in)
    {
        // you update read_fds inside the while loop since whenever a new connection its added to master
        read_fds = master;
        if (select(fdmax + 1, & read_fds, NULL, NULL, NULL) == -1) {
            exit(-1);
        }
        fd=0;
        while(fd<=fdmax)
        {
            if (FD_ISSET(fd, & read_fds))
            {
                //streaming input_ to a socket
                if(fd == input_)
                {
                  char * command = (char * ) malloc(sizeof(char) * MAXX);
                    memset(command, '\0', MAXX);
                    if (fgets(command, MAXX - 1, stdin) != NULL) {
                        execute(command, input_);
                    }
                  fflush(stdout);
                }
                else if (fd == localhost-> file_desc)
                {
                    socklen_t addrlen = sizeof peer;
                    newfd = accept(fd, (struct sockaddr * ) & peer, & addrlen);
                    if (newfd==-1)
                    {
                        exit(EXIT_FAILURE);
                    }
                  fflush(stdout);
                }
                else if (fd == (server->file_desc))
                {
                    //receive data from the server
                    memset(buf,'\0',100000);
                    nbytes = recv(fd, buf, sizeof buf, 0);
                    if (nbytes<=0)
                    {
                        close(fd); // Close the connection
                        FD_CLR(fd, & master); // Remove the fd from master set
                    }
                    else
                    {
                        execute(buf, fd);
                    }

                }

            }
            fd = fd+1;
            fflush(stdout);
        }

    } 
    return;
}

int client_server_connect(char ip[], char port[])
{
    int fd =0;
    int state;
    int server_socket=0, listen_socket =0;
    int yes=1;
    struct addrinfo hints, *res, *servinfo, *clientinfo;
    server = malloc(sizeof(struct host));
    memcpy(server->port_num, port, sizeof(server->port_num));
    memcpy(server -> ip, ip, sizeof(server -> ip));
    memset( & hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    // here the first argument isn't NULL since you are connecting to the server
    state = getaddrinfo(server -> ip, server -> port_num, & hints, & servinfo);
    if(state!=0)
    {
        return 0;
    }
    res = servinfo;
    while(res != NULL){
        /* Socket */
        if ((server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            res= res->ai_next;
            continue;
        }
        // lose the pesky "address already in use" error message
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            return 0;
        }
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1) {   
		return 0;
        }
        if (connect(server_socket, res->ai_addr, res->ai_addrlen) == -1) {
            close(server_socket);
            res = res->ai_next;
            continue;
        }
        break;
    }
    // exit if could not bind
    if (res==NULL)
    {
        return 0;
    }
    server -> file_desc = server_socket;
    state = getaddrinfo(NULL, localhost -> port_num, & hints, & clientinfo);
    if(state!=0)
    {
        return 0;
    }
    res = clientinfo;
    while(res != NULL){
        /* Socket */
        if ((listen_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            res= res->ai_next;
            continue;
        }
        // lose the pesky "address already in use" error message
        if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
          return 0;
        }
        if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1) {
           return 0;
        }
        if (bind(listen_socket, res->ai_addr, res->ai_addrlen) < 0) {
            close(listen_socket);
            res = res->ai_next;
            continue;
        }
        break;
    }
    // exit if could not bind
    if (res==NULL)
    {
       return 0;
    }
    //listen
    if (listen(listen_socket, BACKLOG) == -1) {
        return 0;
    }

    localhost -> file_desc = listen_socket;
    freeaddrinfo(servinfo);
    freeaddrinfo(clientinfo);
    return 1;
}

void server_client_login(char ip[], char port[], char hostname[], int fd)
{
    char response[100000] = "CLIENTLIST\n";
    struct host * temp = clients;
    struct host * prev;
    bool newclient = true;
    struct host * client = malloc(sizeof(struct host));
    int val;
    for(;temp!=NULL;temp=temp->next)
    {
        if(temp-> file_desc == fd)
        {
            newclient = false;
            client = temp;
            break;
        }
        prev = temp;
    }
    if(!newclient)
    {
        client -> logged_in = true;
        
    }
    else
    {
        memcpy(nclient -> hostname, hostname, sizeof(nclient -> hostname));
        memcpy(nclient -> port_num, port, sizeof(nclient -> port_num));
        memcpy(nclient -> ip, ip, sizeof(nclient -> ip));
        client = nclient;
        if(clients == NULL)
        {
            clients = malloc(sizeof(struct host));
            clients = client;
        }
        else if (atoi(port)>atoi(prev->port_num))
        {
            prev -> next = client;
        }
        else if (atoi(port)<atoi(clients->port_num))
        {
            client -> next = clients;
            clients = client;
        }
        else
        {
            struct host * temp = clients -> next;
            struct host * prev = clients;
            while(temp!=NULL)
            {
                if (atoi(port)<atoi(temp->port_num))
                {
                    prev->next = client;
                    client->next = temp;
                    break;
                }
                prev = temp;
                temp = temp->next;

            }
        }
    }
    struct message * messages = client ->queue;
    char answer[100000] = "";
    if(messages){
        strcat(answer,"QUEUEMESSAGES\n");
        while(messages)
    {
        char receive[500 * 5];
        struct host * temp = messages ->sender;
        sprintf(receive, "%s %s\n", temp->ip, messages->msg);
        client -> rcv +=1;
        strcat(answer, receive);
        if(messages->broadcast == false)
        {
            cse4589_print_and_log("[RELAYED:SUCCESS]\n");
            cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", messages -> sender -> ip, ip, messages -> msg);
            cse4589_print_and_log("[RELAYED:END]\n");
        }
        messages = messages -> next_message;
    }
    strcat(answer, "END\n");
    client->queue = NULL;
    }
    for(temp=clients;temp!=NULL;temp=temp->next)
    {
        if(temp-> logged_in==true)
        {
            char var[500 * 5];
            sprintf(var, "%s %s %s\n", temp -> ip, temp -> port_num, temp -> hostname);
            strcat(response, var);
        }
    }
    strcat(response, "END\n");
    strcat(answer,response);
    val = send(fd, answer, strlen(answer) + 1, 0);
    if (val==-1)
    {
        // do nothing print error

    }
}
void server_initialisation(){
    int status; 
    // return value of ggetaddrinfo function is assigned to this
    int server_socket = 0;
    int yes=1; //  for setsockopt() SO_REUSEADDR, below
    //server_socket is the socket file descriptor returned by socket() & servinfo will store the results
    struct addrinfo hints, *res, *servinfo; 
    /* Set up hints structure */
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_family = AF_UNSPEC; //don't care IPv4 or IPv6
    /* Fill up address structures */
    if (getaddrinfo(NULL, localhost -> port_num, &hints, &servinfo) != 0) {
        exit(EXIT_FAILURE);
    }
    // servinfo now points to a linked list of 1 or more struct addrinfos
    // ... do everything until you don't need servinfo anymore ....
    res = servinfo;
    while(res != NULL){
        /* Socket */
        if ((server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            res= res->ai_next;
            continue;
        }
        // lose the pesky "address already in use" error message
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
              exit(EXIT_FAILURE);
        }
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1) {
              exit(EXIT_FAILURE);
        }
        if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1) {
            close(server_socket);
            res = res->ai_next;
            continue;
        }
        break;
    }
     // exit if could not bind
    if (res==NULL)
    {
        exit(EXIT_FAILURE);
    }
    //listen
    if (listen(server_socket, BACKLOG) == -1) {
        exit(EXIT_FAILURE);
    }

    localhost -> file_desc = server_socket;
    freeaddrinfo(res); // free the linked list
    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    int fdmax; // maximum file descriptor number
    int newfd; // newly accept()ed socket descriptor
    int fd=0; // for looping all the file descriptors present in the read_fds
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen; //address length
    char buf[100000]; // buffer for client data
    char remoteIP[INET6_ADDRSTRLEN]; //stores the ip of the client
    int nbytes;
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    fdmax = server_socket;
    FD_SET(input_, & master);
    FD_SET(server_socket, &master);
    for(;;) {
        read_fds = master; // copy it - using the select statement modifies the master 
         if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
                exit(EXIT_FAILURE);
        }
        fd  = 0;
        while(fd<=fdmax)
        {
            // check if the fd is in the read_fds
            if (FD_ISSET(fd, & read_fds)) 
            {

                //streaming input_ to a socket
                if(fd == input_)
                {
                    char * command = (char * ) malloc(sizeof(char) * MAXX);
                    memset(command, '\0', MAXX);
                    if (fgets(command, MAXX - 1, stdin) == NULL) { } 
                    else { execute(command, fd); 
                    fflush(stdout);
                    }
                   
                }

                // if the current file descriptor is the listener that means a new connection has come in
                else if (fd==server_socket)
                {
                    addrlen = sizeof remoteaddr;
                    // new fd is created the server accepts. you need to all the new fd to the master set
                    newfd = accept(server_socket, (struct sockaddr * ) & remoteaddr, & addrlen);
                    if (newfd == -1)
                    {
                        exit(EXIT_FAILURE);
                    }
                    else{
                        nclient = malloc(sizeof(struct host));
                        FD_SET(newfd, & master); // add to master set
                        nclient -> file_desc = newfd;
                        nclient -> logged_in = true;
                        nclient -> next = NULL;
                        memcpy(nclient-> ip, inet_ntop( remoteaddr.ss_family, get_in_addr((struct sockaddr * ) & remoteaddr), remoteIP, INET6_ADDRSTRLEN), sizeof( nclient -> ip));
                        if (newfd>fdmax)
                        {
                            fdmax = newfd;

                        }
                    }
                 fflush(stdout);
                }
                else{
                   memset(buf,'\0',100000);
                    nbytes = recv(fd, buf, sizeof buf, 0);
                    if (nbytes<0) {
                        close(fd); // when the no of bytes you receive from the client is zero or -1(error), you remove the fd from master set
                        FD_CLR(fd,&master);
                    }
                    else
                    {
                        //you need to send fd as a parameter since after the command from the client is executed when need to remove it from file descriptor
                        execute(buf, fd);
                    }
                   fflush(stdout);

                }
            fflush(stdout);
            
        }
        fd+=1;
        }
    
    }
    return;
}

/**cse4589_print_and_log
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

int main(int argc, char ** argv) {
    
    // init log defined in the include/logger file
    cse4589_init_log(argv[2]);

    //Open the logfile and write the port number inside the file
    fclose(fopen(LOGFILE, "w"));

    //We need to make we have three arguments - file name, c/s, port number - otherwise exit
    if (argc != 3) {
        cse4589_print_and_log("enter two argument c/s and PORT number");
		exit(-1);
	}

    populate_host(argv[1],argv[2]);

    return 0;
}


