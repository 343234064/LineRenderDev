// AdjacencyDataGenerator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include "Adjacency.h"

#define LINE_STRING "================================"


void Pass0(AdjacencyProcesser* Processer, std::string& FilePath)
{
    bool Success = false;

    std::filesystem::path VertexFilePath = FilePath;
    VertexFilePath.replace_extension(std::filesystem::path("vertices"));
    Success = Processer->GetReady0(VertexFilePath);


    std::cout << LINE_STRING << std::endl;
    std::cout << Processer->GetMessageString() << std::endl;
    std::cout << LINE_STRING << std::endl;


    if (Success) {
        double Progress = 0.0;
        while (true) {
            std::cout << "Generate Vertex Map....... " << Progress * 100.0 << " %                                                             \r";
            std::cout.flush();

            if (Progress >= 1.0) break;

            Progress = Processer->GetProgress();
        }
        std::cout << std::endl;

        while (true)
        {
            if (Processer->IsWorking())
                Processer->GetProgress();
            else
                break;
        }
    }

    std::string& ErrorString = Processer->GetErrorString();
    if (ErrorString.size() > 0) {
        std::cout << LINE_STRING << std::endl;
        std::cout << "Something get error, please see error0.log." << std::endl;
        std::cout << LINE_STRING << std::endl;

        Processer->DumpErrorString(0);
    }
    std::cout << "Generate Vertex Map Completed." << std::endl;

    /*
    std::vector<VertexContext*> ContextList = Processer.GetVertexContextList();
    VertexContext* Context = ContextList[0];

    ContextList[0]->DumpIndexMap();
    ContextList[0]->DumpVertexList();
    ContextList[0]->DumpRawVertexList();
    */


}


void Pass1(AdjacencyProcesser* Processer, std::string& FilePath)
{
    bool Success = false;

    std::filesystem::path TriangleFilePath = FilePath;
    Success = Processer->GetReady1(TriangleFilePath);
    std::cout << LINE_STRING << std::endl;
    std::cout << Processer->GetMessageString() << std::endl;
    std::cout << LINE_STRING << std::endl;

    if (Success) {
        double Progress = 0.0;
        while (true) {
            std::cout << "Generate Face And Edge Data....... " << Progress * 100.0 << " %                                                             \r";
            std::cout.flush();

            if (Progress >= 1.0) break;

            Progress = Processer->GetProgress();
        }
        std::cout << std::endl;

        while (true)
        {
            if (Processer->IsWorking())
                Processer->GetProgress();
            else
                break;
        }
    }

    std::string& ErrorString = Processer->GetErrorString();
    if (ErrorString.size() > 0) {
        std::cout << LINE_STRING << std::endl;
        std::cout << "Something get error, please see error1.log." << std::endl;
        std::cout << LINE_STRING << std::endl;

        Processer->DumpErrorString(1);
    }
    std::cout << "Generate Face And Edge Data Completed." << std::endl;

}

void Pass2(AdjacencyProcesser* Processer, std::string& FilePath)
{
    bool Success = false;

    std::cout << LINE_STRING << std::endl;
    std::cout << "Begin Generate Adjacency Data..." << std::endl;
    Success = Processer->GetReady2();

    std::cout << LINE_STRING << std::endl;
    std::cout << Processer->GetMessageString() << std::endl;
    std::cout << LINE_STRING << std::endl;

    if (Success) {
        double Progress = 0.0;
        while (true) {
            std::cout << "Generate Adjacency Data....... " << Progress * 100.0 << " %                                                             \r";
            std::cout.flush();

            if (Progress >= 1.0) break;

            Progress = Processer->GetProgress();
        }
        std::cout << std::endl;

        while (true)
        {
            if (Processer->IsWorking())
                Processer->GetProgress();
            else
                break;
        }
    }

    std::string& ErrorString = Processer->GetErrorString();
    if (ErrorString.size() > 0) {
        std::cout << LINE_STRING << std::endl;
        std::cout << "Something get error, please see error2.log." << std::endl;
        std::cout << LINE_STRING << std::endl;

        Processer->DumpErrorString(2);
    }

    std::cout << "Generate Adjacency Data Completed." << std::endl;

}


void Pass3(AdjacencyProcesser* Processer, std::string& FilePath)
{
    bool Success = false;

    std::cout << LINE_STRING << std::endl;
    std::cout << "Begin Shrinking Adjacency Data..." << std::endl;
    Success = Processer->GetReady3();

    std::cout << LINE_STRING << std::endl;
    std::cout << Processer->GetMessageString() << std::endl;
    std::cout << LINE_STRING << std::endl;

    if (Success) {
        double Progress = 0.0;
        while (true) {
            std::cout << "Shrinking Adjacency Data....... " << Progress * 100.0 << " %                                                             \r";
            std::cout.flush();

            if (Progress >= 1.0) break;

            Progress = Processer->GetProgress();
        }
        std::cout << std::endl;

        while (true)
        {
            if (Processer->IsWorking())
                Processer->GetProgress();
            else
                break;
        }
    }

    std::string& ErrorString = Processer->GetErrorString();
    if (ErrorString.size() > 0) {
        std::cout << LINE_STRING << std::endl;
        std::cout << "Something get error, please see error3.log." << std::endl;
        std::cout << LINE_STRING << std::endl;

        Processer->DumpErrorString(3);
    }
    std::cout << "Shrink Adjacency Data Completed." << std::endl;

}

void Pass4(AdjacencyProcesser* Processer, std::string& FilePath)
{
    bool Success = false;

    std::cout << LINE_STRING << std::endl;
    std::cout << "Begin Generate Adjacency Vertex Map..." << std::endl;
    Success = Processer->GetReady4();

    std::cout << LINE_STRING << std::endl;
    std::cout << Processer->GetMessageString() << std::endl;
    std::cout << LINE_STRING << std::endl;

    if (Success) {
        double Progress = 0.0;
        while (true) {
            std::cout << "Generate Adjacency Vertex Map....... " << Progress * 100.0 << " %                                                             \r";
            std::cout.flush();

            if (Progress >= 1.0) break;

            Progress = Processer->GetProgress();
        }
        std::cout << std::endl;

        while (true)
        {
            if (Processer->IsWorking())
                Processer->GetProgress();
            else
                break;
        }
    }

    std::string& ErrorString = Processer->GetErrorString();
    if (ErrorString.size() > 0) {
        std::cout << LINE_STRING << std::endl;
        std::cout << "Something get error, please see error4.log." << std::endl;
        std::cout << LINE_STRING << std::endl;

        Processer->DumpErrorString(4);
    }
    std::cout << "Generate Adjacency Edge List Completed." << std::endl;

}


void Pass5(AdjacencyProcesser* Processer, std::string& FilePath)
{
    bool Success = false;

    std::cout << LINE_STRING << std::endl;
    std::cout << "Begin Serialize Adjacency Vertex Map..." << std::endl;
    Success = Processer->GetReady5();

    std::cout << LINE_STRING << std::endl;
    std::cout << Processer->GetMessageString() << std::endl;
    std::cout << LINE_STRING << std::endl;

    if (Success) {
        double Progress = 0.0;
        while (true) {
            std::cout << "Serialize Adjacency Vertex Map....... " << Progress * 100.0 << " %                                                             \r";
            std::cout.flush();

            if (Progress >= 1.0) break;

            Progress = Processer->GetProgress();
        }
        std::cout << std::endl;

        while (true)
        {
            if (Processer->IsWorking())
                Processer->GetProgress();
            else
                break;
        }
    }

    std::string& ErrorString = Processer->GetErrorString();
    if (ErrorString.size() > 0) {
        std::cout << LINE_STRING << std::endl;
        std::cout << "Something get error, please see error5.log." << std::endl;
        std::cout << LINE_STRING << std::endl;

        Processer->DumpErrorString(5);
    }
    std::cout << "Serialize Adjacency Vertex Map Completed." << std::endl;

}

void Export(AdjacencyProcesser* Processer, std::string& FilePath)
{
    std::cout << LINE_STRING << std::endl;
    std::cout << "Exporting......" << std::endl;

    Processer->ClearMessageString();
    std::filesystem::path ExportFilePath = FilePath;
    ExportFilePath.replace_extension(std::filesystem::path("linemeta"));

    Processer->Export(ExportFilePath);

    std::cout << Processer->GetMessageString() << std::endl;
    std::cout << LINE_STRING << std::endl;

    std::cout << "Export Completed." << std::endl;
}






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
    //FilePath = "F:\\OutlineDev\\LineRenderDev\\LineRenderDev\\Assets\\Models\\MeetMat.triangles";//SphereAndTorus WoodPen BoxAndBox Platonic YJS_Mod
    
    if (FilePath.size() == 0)
    {
        std::cout << "Input File path:" << std::endl;
        std::cin >> FilePath;
    }
    std::cout << LINE_STRING << std::endl;

    {
        AdjacencyProcesser Processer;
        bool Success = false;

        /*****Pass 0*****/////////////////////////////////////////////////////////////////////////////////////////
        /*
        * Merge duplicate vertex
        */
        Pass0(&Processer, FilePath);
        /*****Pass 0*****/////////////////////////////////////////////////////////////////////////////////////////

        /*****Pass 1*****/////////////////////////////////////////////////////////////////////////////////////////
        /*
        * Generate face list and edge list(without repeated)
        */
        Pass1(&Processer, FilePath);
        /*****Pass 1*****/////////////////////////////////////////////////////////////////////////////////////////


        /*****Pass 2*****/////////////////////////////////////////////////////////////////////////////////////////
        /*
        * Generate adjacency face list
        */
        Pass2(&Processer, FilePath);
        /*
        * Generate adjacency edge list(edge with adjacency vertex index)
        */
        /*****Pass 2*****/////////////////////////////////////////////////////////////////////////////////////////


        /*****Pass 3*****/////////////////////////////////////////////////////////////////////////////////////////
        /*
        * Remove repeated adjacency face
        */
        Pass3(&Processer, FilePath);
        /*****Pass 3*****/////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////

        /*****Pass 4*****/////////////////////////////////////////////////////////////////////////////////////////
        /*
        * Generate vertex to adjacency vertex map
        */
        Pass4(&Processer, FilePath);
        /*****Pass 4*****/////////////////////////////////////////////////////////////////////////////////////////

        /*****Pass 5*****/////////////////////////////////////////////////////////////////////////////////////////
        /*
        * Generate vertex to adjacency vertex list(serialize)
        */
        Pass5(&Processer, FilePath);
        /*****Pass 5*****/////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////
        
        //DebugCheck
        //std::vector<SourceContext*> ContextList = Processer.GetTriangleContextList();
        //SourceContext* Context = ContextList[0];
        //ContextList[0]->DumpEdgeList();
        //ContextList[0]->DumpFaceList();
        //ContextList[0]->DumpEdgeFaceList();
        //ContextList[0]->DumpAdjacencyFaceListShrink();
        //ContextList[0]->DumpAdjacencyVertexList();
        //std::vector<VertexContext*> VContextList = Processer.GetVertexContextList();
        //VContextList[0]->DumpIndexMap();
        //VContextList[0]->DumpVertexList();

        Export(&Processer, FilePath);
    }

    if (!SilentMode) {
        char AnyKey[16];
        std::cout << std::endl;
        std::cout << "Input any key to exit" << std::endl;
        std::cin >> AnyKey;
    }
    return 0;
}

