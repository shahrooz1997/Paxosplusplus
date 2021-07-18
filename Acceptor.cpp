/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Acceptor.cpp
 * Author: shahrooz
 * 
 * Created on July 24, 2020, 3:03 PM
 */

#include "Acceptor.h"

using std::cout;
using std::endl;

namespace communicate {
int start_socket(std::string ip, uint16_t port) {
  int server_fd;
  struct sockaddr_in address{};
  struct in_addr faddr{};
  int opt = 1;

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    std::cerr << "socket failed" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &opt, sizeof(opt))) {
    std::cerr << "setsockopt" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (inet_pton(AF_INET, ip.c_str(), &faddr) != 1) {
    std::cerr << "Bad address format" << std::endl;
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = faddr.s_addr;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "bind failed" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 3) < 0) {
    std::cerr << "listen" << std::endl;
    exit(EXIT_FAILURE);
  }

  return server_fd;
}

// Create data based on the received buffer
int read_buffer(char *buffer, uint32_t size, State &s, proposal_id_t *n1,
                proposal_id_t *n2 = nullptr, value_t *v = nullptr) {

  uint32_t i = 0, j = 0;
  for (; i < size && j < sizeof(State); i++) {
    ((char *)(&s))[j++] = buffer[i];
  }

  if (n1 != nullptr) {
    for (j = 0; i < size && j < sizeof(proposal_id_t); i++) {
      ((char *)(n1))[j++] = buffer[i];
    }
  }

  if (n2 != nullptr) {
    for (j = 0; i < size && j < sizeof(proposal_id_t); i++) {
      ((char *)(n2))[j++] = buffer[i];
    }
  }

  if (v != nullptr) {
    for (j = 0; i < size && j < sizeof(bool); i++) {
      ((char *)(&(v->is_nil)))[j++] = buffer[i];
    }
    for (; i < size; i++) {
      v->value.push_back(buffer[i]);
    }
  }

  return 0;
}

// Create the buffer to send based on the inputs
int make_buffer(char *&buffer, uint32_t &size, State s, proposal_id_t *n1,
                proposal_id_t *n2 = nullptr, value_t *v = nullptr) {

  std::vector<char> send_buffer;
  State ret_s = s;
  uint32_t j = 0;
  for (; j < sizeof(State);) {
    send_buffer.push_back(((char *)(&ret_s))[j++]);
  }

  if (n1 != nullptr) {
    for (j = 0; j < sizeof(proposal_id_t);) {
      send_buffer.push_back(((char *)(n1))[j++]);
    }
  }

  if (n2 != nullptr) {
    for (j = 0; j < sizeof(proposal_id_t);) {
      send_buffer.push_back(((char *)(n2))[j++]);
    }
  }

  if (v != nullptr) {
    for (j = 0; j < sizeof(bool);) {
      send_buffer.push_back(((char *)(&(v->is_nil)))[j++]);
    }
    for (j = 0; j < v->value.size();) {
      send_buffer.push_back(v->value[j++]);
    }
  }

  buffer = new char[send_buffer.size()];
  size = send_buffer.size();

  for (uint32_t i = 0; i < size; i++) {
    buffer[i] = send_buffer[i];
  }

  return 0;
}

// Add size of the buffer and send it
int send_buf(int sock, char *buffer, uint32_t size) {

  char *local_buffer = new char[size + 4];

  uint32_t i, j;
  uint32_t temp_size = htonl(size);
  for (i = 0, j = 0; i < 4 && j < sizeof(uint32_t); i++) {
    local_buffer[i] = ((char *)(&temp_size))[j++];
  }

  for (uint32_t i = 4; i < size + 4; i++) {
    local_buffer[i] = buffer[i - 4];
  }

  size += 4;

  uint32_t sent_bytes = 0;
  while (sent_bytes < size) {
    sent_bytes += send(sock, local_buffer + sent_bytes, size - sent_bytes, 0);
  }

  delete[] local_buffer;
  return 0;
}

// receive the buffer based on its size
int receive_buf(int sock, char *&buffer, uint32_t &size) {
  uint32_t valread;
  char buffer_size[4] = {0};

  valread = read(sock, buffer_size, 4);
  while (valread < 4) {
    valread += read(sock, buffer_size + valread, 4 - valread);
  }
  uint32_t rec_size = *((uint32_t *)(buffer_size));
  rec_size = ntohl(rec_size);

  buffer = new char[rec_size];
  size = rec_size;

  valread = read(sock, buffer, size);
  while (valread < size) {
    valread += read(sock, buffer + valread, size - valread);
  }

  return 0;
}

}

void new_connection(Acceptor *acceptor_p, int sock) {

  char *buffer = nullptr;
  uint32_t size;
  State s;
  proposal_id_t n;
  value_t v;
  communicate::receive_buf(sock, buffer, size);
  communicate::read_buffer(buffer, size, s, nullptr, nullptr, nullptr);

  int status = 0;

  if (s != State::Prepare) {
    communicate::read_buffer(buffer, size, s, &n, nullptr, &v);
    status = acceptor_p->accept_received(s, n, v);
    delete buffer;
    buffer = nullptr;

    if (status == 0) {
      State ret_s = State::Accepted;
      communicate::make_buffer(buffer, size, ret_s, &n);
      communicate::send_buf(sock, buffer, size);
    } else {
      State ret_s = State::Nack;
      communicate::make_buffer(buffer, size, ret_s, &n);
      communicate::send_buf(sock, buffer, size);
    }
  } else {
    communicate::read_buffer(buffer, size, s, &n, nullptr, nullptr);
    delete buffer;
    buffer = nullptr;

    status = acceptor_p->prepare_received(s, n);

    if (status == 0) {
      State ret_s = State::Promise;
      communicate::make_buffer(buffer, size, ret_s, &n, &acceptor_p->n_a, &acceptor_p->v_a);
      communicate::send_buf(sock, buffer, size);
    } else {
      State ret_s = State::Nack;
      communicate::make_buffer(buffer, size, ret_s, &n);
      communicate::send_buf(sock, buffer, size);
    }
  }
}

Acceptor::Acceptor(std::string ip, uint16_t port) : ip(ip), port(port) {

  this->n_p.proposer_id = (uint64_t)0;
  this->n_p.round_number = 0;
  this->n_a.proposer_id = (uint64_t)0;
  this->n_a.round_number = 0;
  this->v_a.is_nil = true;
  this->v_a.value.clear();

  int sock = communicate::start_socket(this->ip, this->port);

  while (true) {
    int new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    if ((new_sock = accept(sock, (struct sockaddr *)&address,
                           (socklen_t *)&addrlen)) < 0) {
      std::cerr << "accept error" << std::endl;
      exit(EXIT_FAILURE);
    }

    std::thread th(new_connection, this, new_sock);
    th.detach();
  }

}

Acceptor::~Acceptor() {
}

int Acceptor::prepare_received(State s, proposal_id_t n) {

  std::unique_lock<std::mutex> lock(this->mu_n_p);

  if ((this->n_p.round_number < n.round_number) ||
      (this->n_p.round_number == n.round_number && this->n_p.proposer_id < n.proposer_id)) {
    this->n_p = n;

    // Send back the proposer <Promise, n, n_a, v_a>
    cout << "prepare_received::Promise sent" << endl;
    return 0;
  } else {
    // Send nack
    cout << "prepare_received::Nack sent" << endl;
    return -1;
  }
}

int Acceptor::accept_received(State s, proposal_id_t n, value_t v) {
  std::unique_lock<std::mutex> lock(this->mu_n_p);
  std::unique_lock<std::mutex> lock2(this->mu_n_a);
  if (this->n_p <= n) {
    this->n_p = n;

    this->n_a = n;
    this->v_a = v;

    // Send back the proposer <Accepted, n>
    cout << "accept_received::Accepted sent" << endl;
    return 0;
  } else {
    // Send nack
    cout << "accept_received::Nack sent" << endl;
    return -1;
  }
}

