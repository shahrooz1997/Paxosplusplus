/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main_acceptor.cpp
 * Author: shahrooz
 *
 * Created on August 8, 2020, 3:36 AM
 */

#include <cstdlib>
#include "Acceptor.h"
#include <inttypes.h> /* strtoimax */

using namespace std;

static bool str_to_uint16(const char *str, uint16_t *res) {
  char *end;
  errno = 0;
  intmax_t val = strtoimax(str, &end, 10);
  if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0')
    return false;
  *res = (uint16_t)val;
  return true;
}

/*
 * 
 */
int main(int argc, char **argv) {

  if (argc < 2) {
    exit(-1);
  }

  uint16_t port;
  str_to_uint16(argv[1], &port);

  Acceptor a("127.0.0.1", port); // The server will be started in the constructor

  return 0;
}

