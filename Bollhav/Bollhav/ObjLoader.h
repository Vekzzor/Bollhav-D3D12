#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <DirectXMath.h>

//Print loading process to output window. 
#define CONSOLE_OUTPUT

using namespace DirectX; 

class OBJLoader
{
private: 

	struct CURRENT_VALUES
	{
		std::vector<unsigned int> vertexIndices = {};
		std::vector<unsigned int> uvIndices = {};
		std::vector<unsigned int> normalIndices = {};
		std::vector<XMFLOAT3A> out_vertices = {};
		std::vector<XMFLOAT2A> out_uvs = {};
		std::vector<XMFLOAT3A> out_normals = {};
		std::string status = ""; 
	};

	CURRENT_VALUES m_currentValues = {}; 

public:
	OBJLoader(); 
	~OBJLoader(); 

	/*Load an OBJ file. Returns all the data collected and a status string telling us
	wether everything succeded or not*/
	CURRENT_VALUES loadObj(char* path); 
};

