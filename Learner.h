/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Learner.h
 * Author: shahrooz
 *
 * Created on July 24, 2020, 3:06 PM
 */

#ifndef LEARNER_H
#define LEARNER_H

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

class Learner {
 public:
  Learner(std::string ip, uint16_t port);
  Learner(const Learner &orig) = delete;
  virtual ~Learner() = default;

  int decide_received(State s, value_t v);

 private:
  std::mutex mu_v_d;
  value_t v_d;

  std::string ip;
  uint16_t port;
};

#endif /* LEARNER_H */

