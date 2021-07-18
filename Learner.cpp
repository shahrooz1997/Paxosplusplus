/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Learner.cpp
 * Author: shahrooz
 * 
 * Created on July 24, 2020, 3:06 PM
 */

#include <vector>

#include "Learner.h"

using std::cout;
using std::endl;

namespace communicate{
    int start_socket(std::string ip, uint16_t port){
        int server_fd; 
        struct sockaddr_in address; 
        struct in_addr faddr;
        int opt = 1; 

        // Creating socket file descriptor 
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
            std::cerr << "socket failed" << std::endl;
            exit(EXIT_FAILURE); 
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                      &opt, sizeof(opt))){ 
            std::cerr << "setsockopt" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        if(inet_pton(AF_INET, ip.c_str(), &faddr) != 1){
            std::cerr << "Bad address format" << std::endl;
            exit(EXIT_FAILURE); 
        }

        address.sin_family = AF_INET; 
        address.sin_addr.s_addr = faddr.s_addr;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ 
            std::cerr << "bind failed" << std::endl;
            exit(EXIT_FAILURE); 
        }

        if (listen(server_fd, 3) < 0){ 
            std::cerr << "listen" << std::endl;
            exit(EXIT_FAILURE); 
        }

        return server_fd;
    }
    
    int read_buffer(const char* buffer, uint32_t size, State &s, value_t *v = nullptr){ // Create data based on the received buffer
        
        uint32_t i = 0, j = 0;
        for(; i < size && j < sizeof(State); i++){
            ((char*)(&s))[j++] = buffer[i];
        }

        if(v != nullptr){
            for(j = 0; i < size && j < sizeof(bool); i++){
                ((char*)(&(v->is_nil)))[j++] = buffer[i];
            }

            if(i >= size){
                cout << "Bad Input" << endl;
                return -1;
            }

            for(j = 0; i < size; i++){
        //        ((char*)(&v.is_nil))[j++] = in[i];
                v->value.push_back(buffer[i]);
            }
        }

        return 0;
    }
    
    int make_buffer(char* &buffer, uint32_t &size, State s){ // Create the buffer to send based on the inputs

        std::vector <char> send_buffer;
        State ret_s = s;
        uint32_t j = 0;
        for(;j < sizeof(State);){
            send_buffer.push_back(((char*)(&ret_s))[j++]);
        }

        buffer = new char[send_buffer.size()];
        size = send_buffer.size();

        for(uint32_t i = 0; i < size; i++){
            buffer[i] = send_buffer[i];
        }

        return 0;
    }
    
    int send_buf(int sock, char* buffer, uint32_t size){ // Add size of the buffer and send it
        
        char *local_buffer = new char[size + 4];
        
        uint32_t i, j;
        uint32_t temp_size = htonl(size);
        for(i = 0, j = 0; i < 4 && j < sizeof(uint32_t); i++){
            local_buffer[i] = ((char*)(&temp_size))[j++];
        }

        for(uint32_t i = 4; i < size + 4; i++){
            local_buffer[i] = buffer[i - 4];
        }

        size += 4;

        uint32_t sent_bytes = 0;
        while(sent_bytes < size){
            sent_bytes += send(sock, local_buffer + sent_bytes, size - sent_bytes, 0);
        }

        delete local_buffer;
        return 0;
    }

    int receive_buf(int sock, char* &buffer, uint32_t &size){ // receive the buffer based on its size
        uint32_t valread;
        char buffer_size[4] = {0};

        valread = read(sock, buffer_size, 4);
        while(valread < 4){
            valread += read(sock, buffer_size + valread, 4 - valread);
        }
        uint32_t rec_size = *((uint32_t*)(buffer_size));
        rec_size = ntohl(rec_size);

        buffer = new char[rec_size];
        size = rec_size;
        
        valread = read(sock, buffer, size);
        while(valread < size){
            valread += read(sock, buffer + valread, size - valread);
        }

        return 0;
    }

}

void new_connection(Learner* learner_p, int sock){
    
    char *buffer = nullptr;
//    int status;
    uint32_t size;
    State s;
    value_t v;
    communicate::receive_buf(sock, buffer, size);
    communicate::read_buffer(buffer, size, s);
    
    if(s == State::Decide){
        communicate::read_buffer(buffer, size, s, &v);
        learner_p->decide_received(s, v);
    }
    
    delete buffer;
    buffer = nullptr;
    
    communicate::make_buffer(buffer, size, State::Ack);
    communicate::send_buf(sock, buffer, size);
}

Learner::Learner(std::string ip, uint16_t port): ip(ip), port(port) {
    this->v_d.is_nil = true;
    this->v_d.value.clear();
    
    int sock = communicate::start_socket(ip, this->port);
    
    while(true){
        int new_sock;
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        if ((new_sock = accept(sock, (struct sockaddr *)&address,  
                           (socklen_t*)&addrlen))<0){ 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        }
        
        std::thread th(new_connection, this, new_sock);
        th.detach();
    }
}

Learner::~Learner() {
}

int Learner::decide_received(State s, value_t v){
    std::unique_lock<std::mutex> lock(this->mu_v_d);
    std::cout << "decision received" << std::endl;
    if(this->v_d.is_nil){
        this->v_d = v;
    }
    
    std::cout << "received value: \"";
    for(uint32_t i = 0; i < this->v_d.value.size(); i++){
        std::cout << this->v_d.value[i];
    }
    std::cout << "\"" << endl;
    
    return 0;
}
