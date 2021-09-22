// AdjacencyDataGenerator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include "Adjacency.h"

#define LINE_STRING "================================"

int main(int argc, char** argv)
{
    bool SilentMode = false;
    std::string FilePath;
    std::cout << LINE_STRING << std::endl;
    if (argc == 1)
    {
        std::cout << "No argument\n";
    }
    else
    {
        if (strcmp(argv[1], "-s") == 0)
        {
            SilentMode = true;
            FilePath = argv[2];
        }
        else
        {
            FilePath = argv[1];
        }
        std::cout << FilePath.c_str() << std::endl;
    }
    //temp
    FilePath = "F:\\OutlineDev\\LineRenderDev\\LineRenderDev\\Assets\\Models\\SphereAndTorus.triangles";
    
    if (FilePath.size() == 0)
    {
        std::cout << "Input File path:" << std::endl;
        std::cin >> FilePath;
    }
    std::cout << LINE_STRING << std::endl;

    {
        AdjacencyProcesser Processer;
        bool Success = Processer.GetReady(FilePath);

        std::cout << LINE_STRING << std::endl;
        std::cout << Processer.GetMessageString() << std::endl;
        std::cout << LINE_STRING << std::endl;

        if (Success) {
            double Progress = 0.0;
            while (true) {
                std::cout << "Processing....... " << Progress * 100.0 << " %                                                             \r";
                std::cout.flush();

                if (Progress >= 1.0) break;

                Progress = Processer.GetProgress();
            }
            std::cout << std::endl;

            while (true)
            {
                if (Processer.IsWorking())
                    Processer.GetProgress();
                else
                    break;
            }
        }

        std::cout << LINE_STRING << std::endl;
        std::cout << Processer.GetErrorString() << std::endl;
        std::cout << LINE_STRING << std::endl;

        // temp
        std::vector<SourceContext*>& ContextList = Processer.GetContextList();
        for (int i = 0; i < ContextList.size(); i++) {
            std::cout << LINE_STRING << std::endl;
            std::cout << ContextList[i]->FaceList.size() << std::endl;
            std::cout << ContextList[i]->EdgeList.size() << std::endl;
            std::cout << LINE_STRING << std::endl;
        }
    }

    if (!SilentMode) {
        char AnyKey[16];
        std::cout << std::endl;
        std::cout << "Input any key to exit" << std::endl;
        std::cin >> AnyKey;
    }
    return 0;
}

