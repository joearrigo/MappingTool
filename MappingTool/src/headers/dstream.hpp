#ifndef DSTREAM_HPP
#define DSTREAM_HPP

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#define DEBUG_ON 1
#define DEBUG_OFF 0

extern std::stringstream lout;
extern std::stringstream dout;
extern int debugStatus;
void updConsole();
void setDebug(int in);
#endif
