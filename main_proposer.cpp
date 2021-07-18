/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main_proposer.cpp
 * Author: shahrooz
 *
 * Created on August 8, 2020, 3:36 AM
 */

#include <cstdlib>
#include <vector>
#include "Proposer.h"
#include <sstream>      // std::istringstream
#include <string>       // std::string

using namespace std;

/*
 * 
 */
int main(int argc, char **argv) {

  if (argc < 2) {
    exit(-1);
  }

  // Set ip addresses
  for (uint16_t i = 0; i < 3; i++) {
    acceptors.push_back(new Server(std::string("127.0.0.1"), i + 12000));
  }
  for (uint16_t i = 0; i < 2; i++) {
    learners.push_back(new Server(std::string("127.0.0.1"), i + 13000));
  }

  uint64_t id;
  std::istringstream iss(argv[1]);
  iss >> id;

  Proposer p(id);
  cout << "Proposer Created" << endl;
  value_t v;
  v.is_nil = false;
  for (int i = 0; i < 20; i++)
    v.value.push_back('a' + (id - 1) * 2);
  while (p.propose(v) != 0);

  v.value.clear();
  for (int i = 0; i < 20; i++)
    v.value.push_back('b' + (id - 1) * 2);
  while (p.propose(v) != 0);

  return 0;
}

