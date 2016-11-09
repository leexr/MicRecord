#pragma once
#include <Windows.h>
#include <exception>
#include <string>

class RuntimeException {
public:
	RuntimeException() = delete;
	virtual ~RuntimeException() = delete;
	static void mmErrorThrow(MMRESULT result) {
		if (MMSYSERR_NOERROR != result) {
			char buf[1 << 10];
			if (MMSYSERR_NOERROR == waveInGetErrorTextA(result, buf, 1 << 10)) {
				throw new std::runtime_error(buf);
			}
			throw std::runtime_error("unknow error");
		}
	}

	static void Win32ErrorThrow(DWORD dw = ::GetLastError()) {
		LPVOID lpMsgBuf;
		if (dw == 0) {
			return;
		}
		if (!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&lpMsgBuf,
			0,
			NULL)) {
			throw std::runtime_error("unkbow error");
		}
		std::string err((char *)lpMsgBuf);
		LocalFree(lpMsgBuf);
		throw std::runtime_error(err);
	}
};

