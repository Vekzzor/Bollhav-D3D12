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

	std::vector<unsigned int> m_vertexIndices, m_uvIndices, m_normalIndices; 
	std::vector<XMFLOAT3A> m_tempVertices; 
	std::vector<XMFLOAT2A> m_tempUVs; 
	std::vector<XMFLOAT3A> m_tempNormals;

public:
	OBJLoader(); 
	~OBJLoader(); 

	//Load an OBJ file
	bool loadObj(char* path,
		std::vector<XMFLOAT3A>& out_vertices,
		std::vector<XMFLOAT2A>& out_uvs,
		std::vector<XMFLOAT3A>& out_normals); 

};

