/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Proposer.h
 * Author: shahrooz
 *
 * Created on July 24, 2020, 2:54 PM
 */

#ifndef PROPOSER_H
#define PROPOSER_H

#include "Utils.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <arpa/inet.h>

extern std::vector <Server*> acceptors;
extern std::vector <Server*> learners;

class Proposer {
public:
    Proposer(uint64_t id);
    Proposer(const Proposer& orig) = delete;
    virtual ~Proposer();
    
    int propose(value_t c);
    
    std::mutex prepare_phase_mu;
    std::mutex accept_phase_mu;
    
private:
    
    uint64_t id;
    proposal_id_t n_c;
    value_t v_c;
};

#endif /* PROPOSER_H */

