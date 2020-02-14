#include "AttysComm.h"

#include <assert.h>

#include <chrono>
#include <thread>

void AttysComm::connect() {
	if (btAddr == NULL) throw "Bluetooth structure is NULL";
	if (btAddrLen == 0) throw "Bluetooth structure length is zero.";

	// allocate a socket
#ifdef __linux__ 
	btsocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
#elif _WIN32
	btsocket = ::socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	if (INVALID_SOCKET == btsocket) {
		_RPT0(0, "=CRITICAL= | socket() call failed.\n");
		throw "socket() call failed: cannot create socket!";
	}
#endif

	// connect to server
	int status = ::connect(btsocket, btAddr, btAddrLen);

	if (status == 0) {
		_RPT0(0, "Connect successful\n");
		if (attysCommMessage) {
			attysCommMessage->hasMessage(MESSAGE_CONNECTED, "Connected");
		}
		return;
	}
	_RPT1(0, "Connect failed: status=%d\n",status);
	if (attysCommMessage) {
		attysCommMessage->hasMessage(MESSAGE_ERROR, "Connect failed");
	}
	closeSocket();
	throw "Connect failed";
}


void AttysComm::closeSocket() {
#ifdef __linux__ 
	shutdown(btsocket, SHUT_RDWR);
	close(btsocket);
#elif _WIN32
	shutdown(btsocket, SD_BOTH);
	closesocket(btsocket);
#else
#endif
}


void AttysComm::sendSyncCommand(const char *message, int waitForOK) {
	char cr[] = "\n";
	int ret = 0;
	// 10 attempts
	for (int k = 0; k < 10; k++) {
		_RPT1(0, "Sending: %s", message);
		ret = send(btsocket, message, (int)strlen(message), 0);
		if (ret < 0) {
			if (attysCommMessage) {
				attysCommMessage->hasMessage(errno, "message transmit error");
			}
		}
		send(btsocket, cr, (int)strlen(cr), 0);
		if (!waitForOK) {
			return;
		}
		for (int i = 0; i < 100; i++) {
			char linebuffer[8192];
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			ret = recv(btsocket, linebuffer, 8191, 0);
			if (ret < 0) {
				if (attysCommMessage) {
					attysCommMessage->hasMessage(errno, "could receive OK");
				}
			}
			if ((ret > 2) && (ret < 5)) {
				linebuffer[ret] = 0;
				//fprintf(stderr,">>%s<<\n",linebuffer);
				linebuffer[ret] = 0;
				if (strstr(linebuffer, "OK")) {
					_RPT0(0, " -- OK received\n");
					return;
				}
			}
		}
		_RPT0(0, " -- no OK received!\n");
	}
}


void AttysComm::sendInit() {
	_RPT0(0,"Sending Init\n");
	// flag to prevent the data receiver to mess it up!
	initialising = 1;
#ifdef _WIN32
	u_long iMode = 1;
	ioctlsocket(btsocket, FIONBIO, &iMode);
#else
	fcntl(btsocket, F_SETFL, fcntl(btsocket, F_GETFL, 0) | O_NONBLOCK);
#endif
	strcpy(inbuffer, "");
	char rststr[] = "\n\n\n\n\r";
	send(btsocket, rststr, (int)strlen(rststr), 0);

	// everything platform independent
	sendInitCommandsToAttys();

#ifdef _WIN32
	iMode = 0;
	ioctlsocket(btsocket, FIONBIO, &iMode);
#else
	fcntl(btsocket, F_SETFL, fcntl(btsocket, F_GETFL, 0) & ~O_NONBLOCK);
#endif
	strcpy(inbuffer, "");
	initialising = 0;
	_RPT0(0,"Init finished. Waiting for data.\n");
}


void AttysComm::run() {
	char recvbuffer[8192];

	sendInit();

	doRun = 1;

	if (attysCommMessage) {
		attysCommMessage->hasMessage(MESSAGE_RECEIVING_DATA, "Connected");
	}

	watchdogCounter = TIMEOUT_IN_SECS * getSamplingRateInHz();
	watchdog = new std::thread(AttysComm::watchdogThread, this);

	// Keep listening to the InputStream until an exception occurs
	while (doRun) {

		while (initialising && doRun) {
			Sleep(100);
		}
		int ret = recv(btsocket, recvbuffer, sizeof(recvbuffer), 0);
		if (ret < 0) {
			if (attysCommMessage) {
				attysCommMessage->hasMessage(errno, "data reception error");
			}
		}
		if (ret > 0) {
			processRawAttysData(recvbuffer, ret);
		}
	}

	watchdog->join();
	delete watchdog;
	watchdog = NULL;
};
