/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Acceptor.h
 * Author: shahrooz
 *
 * Created on July 24, 2020, 3:03 PM
 */

#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "Utils.h"
#include <thread>
#include <mutex>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>


class Acceptor {
public:
    Acceptor(std::string ip, uint16_t port);
    Acceptor(const Acceptor& orig) = delete;
    virtual ~Acceptor();
    
    int prepare_received(State s, proposal_id_t n);
    int accept_received(State s, proposal_id_t n, value_t v);
    
//private:
    
    std::string ip;
    uint16_t port;

    std::mutex mu_n_p, mu_n_a;
    proposal_id_t n_p;
    proposal_id_t n_a;
    value_t v_a;
};

#endif /* ACCEPTOR_H */

