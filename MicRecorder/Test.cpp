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
    DeleteFileW(L"c:\\temp\\111.wav");
    try {
		MicRecorder *recoder = new MicRecorder;
        recoder->Start(L"c:\\temp\\111.wav");
		auto s = time(NULL);
        auto a = 0;
        std::cin >> a;
        recoder->Stop();
		std::cout << time(NULL) - s << std::endl;
        delete recoder;
    }
    catch (std::exception &e) {
        std::cout << e.what();
    }
	std::vector<std::wstring> devs;
    MicRecorder::EnumDevices(devs);
	system("pause");
    return 0;
}