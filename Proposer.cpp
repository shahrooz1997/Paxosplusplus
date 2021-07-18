/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Proposer.cpp
 * Author: shahrooz
 * 
 * Created on July 24, 2020, 2:54 PM
 */

#include <vector>

#include "Proposer.h"

std::vector <Server*> acceptors;
std::vector <Server*> learners;

using std::cout;
using std::endl;

Proposer::Proposer(uint64_t id): id(id) {
    
    this->n_c.proposer_id = this->id;
    this->n_c.round_number = 0;
    this->v_c.is_nil = true;
    this->v_c.value.clear();
    
}

Proposer::~Proposer() {
};

namespace communicate{
    
    class Connect{
    public:
        
        Connect(const std::string ip, const uint16_t port): ip(ip), port(port),
                            sock(0), connected(false){
            
            struct sockaddr_in serv_addr;
            if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                std::cerr << "Socket creation error" << std::endl;
            }
            else{
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(port);
                std::string ip_str = ip;

                if(inet_pton(AF_INET, ip_str.c_str(), &serv_addr.sin_addr) <= 0){
                    std::cerr << "Invalid address/ Address not supported" << std::endl;
                }
                else{
                    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
                        std::cerr << "Connection Failed" << std::endl;
                    }
                    else{
                        connected = true;
                    }
                }
            }
        }
        
        Connect(const Connect& orig) = delete;
        
        ~Connect(){
            close(sock);
        }
        
        std::string get_ip(){
            return ip;
        }
        
        uint16_t get_port(){
            return port;
        }
        
        bool is_connected(){
            return connected;
        }
        
        int operator*(){
            if(is_connected())
                return this->sock;
            else{
                std::cerr << "Connection is not created!" << std::endl;
                return -1;
            }
        }
        
    private:
        
        std::string ip;
        uint16_t    port;
        
        int sock;
        
        bool connected;
    };
    
    int make_buffer(char* &buffer, uint32_t &size, State s, proposal_id_t *n1,
                proposal_id_t *n2 = nullptr, value_t *v = nullptr){ // Create the buffer to send based on the inputs

        std::vector <char> send_buffer;
        State ret_s = s;
        uint32_t j = 0;
        for(;j < sizeof(State);){
            send_buffer.push_back(((char*)(&ret_s))[j++]);
        }

        if(n1 != nullptr){
            for(j = 0; j < sizeof(proposal_id_t);){
                send_buffer.push_back(((char*)(n1))[j++]);
            }
        }

        if(n2 != nullptr){
            for(j = 0; j < sizeof(proposal_id_t);){
                send_buffer.push_back(((char*)(n2))[j++]);
            }
        }

        if(v != nullptr){
            for(j = 0; j < sizeof(bool);){
                send_buffer.push_back(((char*)(&(v->is_nil)))[j++]);
            }
            for(j = 0; j < v->value.size();){
                send_buffer.push_back(v->value[j++]);
            }
        }

        buffer = new char[send_buffer.size()];
        size = send_buffer.size();

        for(uint32_t i = 0; i < size; i++){
            buffer[i] = send_buffer[i];
        }

        return 0;
    }

    int read_buffer(char* buffer, uint32_t size, State &s, proposal_id_t *n1,
            proposal_id_t *n2 = nullptr, value_t *v = nullptr){ // Create data based on the received buffer

        uint32_t i = 0, j = 0;
        for(; i < size && j < sizeof(State); i++){
            ((char*)(&s))[j++] = buffer[i];
        }

        if(n1 != nullptr){
            for(j = 0; i < size && j < sizeof(proposal_id_t); i++){
                ((char*)(n1))[j++] = buffer[i];
            }
        }

        if(n2 != nullptr){
            for(j = 0; i < size && j < sizeof(proposal_id_t); i++){
                ((char*)(n2))[j++] = buffer[i];
            }
        }

        if(v != nullptr){
            for(j = 0; i < size && j < sizeof(bool); i++){
                ((char*)(&(v->is_nil)))[j++] = buffer[i];
            }
            for(; i < size; i++){
                v->value.push_back(buffer[i]);
            }
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

typedef struct Acceptor_response{
    proposal_id_t n;
    value_t v;
    bool operator<(Acceptor_response &a){
        if((this->n.round_number < a.n.round_number) || 
                (this->n.round_number == a.n.round_number && this->n.proposer_id < a.n.proposer_id)){
            return true;
        }
        return false;
    }
    
    static uint32_t find_max(std::vector <Acceptor_response> &s){
        uint32_t max_i = 0;
        for(uint32_t i = 1; i < s.size(); i++){
            if(s[max_i] < s[i]){
                max_i = i;
            }
        }

        return max_i;
    }
    
} acceptor_response_t;

void _send_prepare(proposal_id_t n_c,
                    std::condition_variable *cv, uint32_t *counter, Server *server,
                    std::vector <acceptor_response_t> *S, bool *done, Proposer *proposer_p){
    
    cout << "_send_prepare called" << endl;
    
    communicate::Connect c(server->ip, server->port);
    if(!c.is_connected()){
        return;
    }
    
    char *buffer = nullptr;
    uint32_t size;
    communicate::make_buffer(buffer, size, State::Prepare, &n_c);
    communicate::send_buf(*c, buffer, size);
    
    delete buffer;
    buffer = nullptr;
    
    State s;
    proposal_id_t n1;
    proposal_id_t n2;
    value_t v;
    communicate::receive_buf(*c, buffer, size);
    communicate::read_buffer(buffer, size, s, nullptr);
    
    if(s == State::Promise){
        
        communicate::read_buffer(buffer, size, s, &n1, &n2, &v);
        buffer[size] = '\0';
        
        std::stringstream msg;
        msg << "Promise received on propose_id: " << n1.proposer_id << "_" <<
                        n1.round_number << " n2: " <<  n2.proposer_id << "_" <<
                        n2.round_number << " v: " << v.is_nil << "_" << v.value.size() << std::endl;
        std::cout << msg.str();
        
        acceptor_response_t one_resp = {n2, v};
        if(n_c == n1){
            std::unique_lock<std::mutex> lock(proposer_p->prepare_phase_mu);
            if(!(*done)){
                S->push_back(one_resp);
                (*counter)++;
                cv->notify_one();
            }
        }
    }
    else if(s == State::Nack){
        communicate::read_buffer(buffer, size, s, &n1);
        std::stringstream msg;
        msg << "Nack received on propose_id: " << n1.proposer_id << "_" <<
                        n1.round_number << std::endl;
        std::cout << msg.str();
        if(n_c == n1){
            std::unique_lock<std::mutex> lock(proposer_p->prepare_phase_mu);
            if(!(*done)){
                *done = true;
                cv->notify_one();
            }
        }
        
    }
    
    delete buffer;
    
    return;
}

void _send_accept(proposal_id_t n_c, value_t v_c,
                    std::condition_variable *cv, uint32_t *counter, Server *server,
                    bool *done, Proposer *proposer_p){
    
    cout << "_send_accept called" << endl;
    
    communicate::Connect c(server->ip, server->port);
    if(!c.is_connected()){
        return;
    }
    
    char *buffer = nullptr;
    uint32_t size;
    communicate::make_buffer(buffer, size, State::Accept, &n_c, nullptr, &v_c);
    communicate::send_buf(*c, buffer, size);
    delete buffer;
    buffer = nullptr;
    
    State s;
    proposal_id_t n1;
    communicate::receive_buf(*c, buffer, size);
    communicate::read_buffer(buffer, size, s, &n1);
    
    if(s == State::Accepted){
        if(n_c == n1){
            
            std::stringstream msg;
            msg << "Accepted received on propose_id: " << n1.proposer_id << "_" <<
                            n1.round_number << std::endl;
            std::cout << msg.str();
            
            std::unique_lock<std::mutex> lock(proposer_p->accept_phase_mu);
            if(!(*done)){
                (*counter)++;
                cv->notify_one();
            }
        }
    }
    else if(s == State::Nack){
        if(n_c == n1){
            std::stringstream msg;
            msg << "Nack received on propose_id: " << n1.proposer_id << "_" <<
                            n1.round_number << std::endl;
            std::cout << msg.str();
            std::unique_lock<std::mutex> lock(proposer_p->prepare_phase_mu);
            if(!(*done)){
                *done = true;
                cv->notify_one();
            }
        }
        
    }
    
    delete buffer;
    
    return;
}

void _send_decide(value_t v_c,
                    Server *server,
                    Proposer *proposer_p){
    
    cout << "_send_decide called" << endl;
    
    communicate::Connect c(server->ip, server->port);
    if(!c.is_connected()){
        return;
    }
    
    char *buffer = nullptr;
    uint32_t size = 0;
    communicate::make_buffer(buffer, size, State::Decide, nullptr, nullptr, &v_c);
    cout << "size is " << size << endl;
    communicate::send_buf(*c, buffer, size);
    delete buffer;
    buffer = nullptr;
    
    State s;
//    proposal_id_t n1;
//    proposal_id_t n2;
//    value_t v;
    communicate::receive_buf(*c, buffer, size);
    communicate::read_buffer(buffer, size, s, nullptr);
    
    if(s == State::Ack){
//        acceptor_response_t one_resp = {n2, v};
        std::cout << "One learner notified" << std::endl;
    }
    
    return;
}

int Proposer::propose(value_t c){
    
    using std::cout;
    using std::endl;
    
    proposal_id_t n_c;
    n_c.proposer_id = this->n_c.proposer_id;
    n_c.round_number = ++this->n_c.round_number;
    
    std::vector <acceptor_response_t> S;
    
    // Send <prepare, n_c> to all acceptors and fill S in.
    uint32_t counter = 0;
    std::condition_variable cv;
    static bool done = false;
    
    done = false;

    for(std::vector<Server*>::iterator it = acceptors.begin();
            it != acceptors.end(); it++){
        std::thread th(_send_prepare, n_c, &cv, &counter,
                *it, &S, &done, this);
        th.detach();
    }
    
    cout << "Prepare message senders created." << endl;

    // Conditional wait on the number of members in S
    std::unique_lock<std::mutex> lock(this->prepare_phase_mu);
    while((counter < acceptors.size() / 2 + 1) && !done){
        cv.wait(lock);
    }
    
    if(done){ // Nack received
        cout << "Nack received in prepare phase" << endl;;
        return -1;
    }
    
    cout << "enough promise received." << endl;
    
    done = true;
    
    acceptor_response_t res;
    res = S[Acceptor_response::find_max(S)];
    if(!res.v.is_nil){
        cout << "consesus is NOT ours" << endl;
        this->v_c = res.v;
    }
    else{
        cout << "consesus is ours" << endl;
        this->v_c = c;
    }
    
    lock.unlock();
    
    cout << "accept phase started" << endl;
    
    // Send <Accept, n_c, v_c> to all acceptors
    uint32_t counter2 = 0;
//    std::mutex mtx;
    std::condition_variable cv2;
    static bool done2 = false;
    
    done2 = false;

    for(std::vector<Server*>::iterator it = acceptors.begin();
            it != acceptors.end(); it++){
        std::thread th(_send_accept, n_c, this->v_c, &cv2, &counter2,
                *it, &done2, this);
        th.detach();
    }
    
    cout << "Accept message senders created." << endl;
    
    // Majority response
    std::unique_lock<std::mutex> lock2(this->accept_phase_mu);
    while((counter2 < acceptors.size() / 2 + 1) && !done2){
        cv2.wait(lock2);
    }
    
    if(done2){ // Nack received
        cout << "Nack received in accept phase" << endl;;
        return -1;
    }
    
    done2 = true;
    lock2.unlock();
    
    cout << "enough accept message received." << endl;
    
    // Send <decide, v_c> to learners
    std::vector <std::thread*> learner_threads;
    for(std::vector<Server*>::iterator it = learners.begin();
            it != learners.end(); it++){
//        std::thread th(_send_decide, this->v_c, *it, this);
        learner_threads.push_back(new std::thread(_send_decide, this->v_c, *it, this));
    }
    
    for(uint32_t i = 0; i < learners.size(); i++){
        learner_threads[i]->join();
    }
    
    return 0;
}

