// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <Windows.h>
#include <sstream>
#include "MicRecorder.h"

#pragma comment(lib,"winmm.lib")

int main() {
    DeleteFileW(L"c:\\temp\\123.wav");
	MicRecorder *recoder = new MicRecorder;
    try {
        recoder->Start(L"c:\\temp\\123.wav");
        auto a = 0;
        std::cin >> a;
        recoder->Stop();
        delete recoder;
    }
    catch (std::exception &e) {
        std::cout << e.what();
    }
	std::vector<std::wstring> devs;
    MicRecorder::EnumDevices(devs);
    return 0;
}



