// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include "MicRecorder.h"
#include "WaveFilePlayer.h"

int main() {
	try {
		WaveFilePlayer p(GetConsoleWindow());
		MicRecorder r;

		std::string command;
		while (true) {
			std::cout << "input q:quit" << std::endl
					  << "input record:begin record" << std::endl
					  << "input stoprecord: stop srop record" << std::endl
					  << "input load: load wav file" << std::endl
					  << "input detach: detach loaded wav file" << std::endl
					  << "input play: paly loaded wav file" << std::endl
					  << "input stop: stop wav file" << std::endl
					  << "input pause: pause wav file play" << std::endl
					  << "input volume: set player volume" << std::endl;

			std::cin >> command;
			try {
				if ("q" == command) {
					return 0;
				} else if ("record" == command) {
					std::cout << "input record file path" << std::endl;
					std::wstring file;
					std::wcin >> file;
					r.Start(file);
				} else if ("stoprecord" == command) {
					r.Stop();
				} else if ("load" == command) {
					std::cout << "input wav file path" << std::endl;
					std::wstring file;
					std::wcin >> file;
					p.Load_fileByPath(file);
					std::cout << "duration" <<p.WaveDuration() << std::endl;
					::Sleep(1000);
				} else if ("play" == command) {
					p.Play();
				} else if ("stop" == command) {
					p.Stop();
				} else if ("pause" == command) {
					p.Pause();
				} else if ("detach" == command) {
					p.Detach_file();
				} else if ("volume" == command) {
					std::cout << "input volume range 0 - 100:" << std::endl;
					short v = 0;
					std::cin >> v;
					p.SetVolume(v);
				}
			} catch (std::exception e) {
				std::cout << e.what() << std::endl;
				system("pause");
			}
			system("cls");
		}
		system("pause");
	} catch (std::exception &e) {
		std::cout << "WAVE ERROR:" << e.what() << "\b Terminate" <<std::endl;
		system("pause");
	}
    return 0;
}