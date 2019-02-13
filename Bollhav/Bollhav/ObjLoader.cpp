#include "ObjLoader.h"


OBJLoader::OBJLoader()
{
}

OBJLoader::~OBJLoader()
{
}

bool OBJLoader::loadObj(char * path,
	std::vector<XMFLOAT3A>& out_vertices, 
	std::vector<XMFLOAT2A>& out_uvs, 
	std::vector<XMFLOAT3A>& out_normals)
{
	std::ifstream objFile; 
	objFile.open(path); 

	if (objFile.is_open())
	{
		char type[128]; 

		while (!objFile.eof)
		{
			objFile >> type; 

			if (type == "v")
			{
				XMFLOAT3A vertex; 

				/*fscanf: Which file are we reading from, read 3 (or 2) floats, where shall the data be stored*/

				fscanf(dynamic_cast<FILE*>(&objFile), "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z); 
				m_vertices.push_back(vertex); 
			}
			else if (type == "vt")
			{
				XMFLOAT2A uv; 
				fscanf(dynamic_cast<FILE*>(&objFile), "%f %f\n", &uv.x, &uv.y); 
				m_uvs.push_back(uv); 
			}
			else if(type == "vn")
			{
				XMFLOAT3A normal; 
				fscanf(dynamic_cast<FILE*>(&objFile), "%f %f %f\n", normal.x, normal.y, normal.z);
				m_normals.push_back(normal); 
			}
			else if (type == "f")
			{
				std::string vert1, vert2, vert3;
				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

				/*This data corresponds to a face, stores the position of its verticed, its UV's and its normals.
				fscanf returns the amount of succesfully converted and assigned fields; meaning that if
				we do not get 9 matches, the data is faulty.*/

				int matches = fscanf(dynamic_cast<FILE*>(&objFile), "%d/%d/%d %d/%d/%d %d/%d/%d\n",
					&vertexIndex[0], &uvIndex[0], &normalIndex[0],
					&vertexIndex[1], &uvIndex[1], &normalIndex[1],
					&vertexIndex[2], &uvIndex[2], &normalIndex[2]);

				if (matches != 9)
				{
					std::cout << "File can't be read, format does not match." << std::endl;
					return false;
				}

				/*Store the indices, the 3 vertex indeces, the 2 UV indices and the
				3 normal indices. Store them in seperate arrays.*/
				m_vertexIndices.push_back(vertexIndex[0]);
				m_vertexIndices.push_back(vertexIndex[1]);
				m_vertexIndices.push_back(vertexIndex[2]);

				m_uvIndices.push_back(uvIndex[0]);
				m_uvIndices.push_back(uvIndex[1]);
				m_uvIndices.push_back(uvIndex[2]);

				m_normalIndices.push_back(normalIndex[0]);
				m_normalIndices.push_back(normalIndex[1]);
				m_normalIndices.push_back(normalIndex[2]);

			}
		}
	}
	else
	{
		std::cout << "Unable to open OBJ-file!" << std::endl; 
		return false; 
	}
}
