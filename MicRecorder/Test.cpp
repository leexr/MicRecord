// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include "MicRecorder.h"
#include "WaveFilePlayer.h"

int main() {
	WaveFilePlayer p(GetConsoleWindow());
    MicRecorder r;
    
    std::string command;
    while (true)
    {
        std::cout << "input q:quit" << std::endl
                  << "input record:begin record" << std::endl
                  << "input stoprecord: stop srop record" << std::endl
                  << "input load: load wav file" << std::endl
                  << "input detach: detach loaded wav file" << std::endl
                  << "input play: paly loaded wav file" << std::endl
                  << "input stop: stop wav file" << std::endl
                  << "input pause : pause wav file play" << std::endl;
        std::cin >> command;
        try {
            if ("q" == command) {
                return 0;
            }
            if ("record" == command) {
                std::cout << "input record file path" << std::endl;
                std::wstring file;
                std::wcin >> file;
                r.Start(file);
            }
            if ("stoprecord" == command) {
                r.Stop();
            }
            if ("load" == command) {
                std::cout << "input wav file path" << std::endl;
                std::wstring file;
                std::wcin >> file;
                p.Load_file(file);
            }
            if ("play" == command) {
                p.Play();
            }
            if ("stop" == command) {
                p.Stop();
            }
            if ("pause" == command) {
                p.Pause();
            }
            if ("detach" == command) {
                p.Detach_file();
            }
        }
        catch (std::exception e) {
            std::cout << e.what() << std::endl;
            system("pause");
        }
        system("cls");
    }
	system("pause");
    return 0;
}