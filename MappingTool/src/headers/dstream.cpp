#include "dstream.hpp"

std::stringstream lout;
std::stringstream dout;
int debugStatus = 0;

void updConsole() {
	std::string consoleLog = lout.str();
	if (consoleLog.length() > 0) {
		std::cout << consoleLog;
		lout.str("");
	}
	consoleLog = dout.str();
	if (consoleLog.length() > 0 && debugStatus == 1) {
		std::cout << consoleLog;
		dout.str("");
	}
}