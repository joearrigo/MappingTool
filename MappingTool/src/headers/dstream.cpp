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

void setDebug(int in) {
	if (in != 0 && in != 1) {
		dout << "Invalid debugStatus value: " << in << ". Use DEBUG_ON, or DEBUG_OFF" << "\n";
		return;
	}
	debugStatus = in;
}