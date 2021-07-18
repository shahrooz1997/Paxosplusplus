/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Utils.h
 * Author: shahrooz
 *
 * Created on July 24, 2020, 2:56 PM
 */

#ifndef UTILS_H
#define UTILS_H

#define LEARNER_PORT 8083

#include <utility>
#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <sstream>

typedef struct Value_t {
  bool is_nil;
  std::vector<char> value;
} value_t;

typedef struct Proposal_id {
  uint64_t round_number;
  uint64_t proposer_id;
  bool operator==(Proposal_id &p) const {
    if (this->round_number == p.round_number && this->proposer_id == p.proposer_id)
      return true;
    return false;
  }
  bool operator<(Proposal_id &p) const {
    if ((this->round_number < p.round_number) || (this->round_number == p.round_number
        && this->proposer_id < p.proposer_id))
      return true;
    return false;
  }
  bool operator<=(Proposal_id &p) const {
    if (*this == p)
      return true;
    if (*this < p)
      return true;
    return false;
  }
} proposal_id_t;

enum State {
  Prepare = 0, Promise, Accept, Accepted, Decide, Nack, Ack
};

struct Server {
  std::string ip;
  uint16_t port;
  Server(std::string ip, uint16_t port) : ip(std::move(ip)), port(port) {}
};

//std::vector <Server*> proposers;

#endif /* UTILS_H */

