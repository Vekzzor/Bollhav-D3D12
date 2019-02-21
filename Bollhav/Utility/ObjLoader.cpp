#include "ObjLoader.h"

#define SM0 "ERROR: File could not be loaded.";
#define SM1 "ERROR: Failed to collect index data.";
#define SM2 "ERROR: Insufficent data recorded.";
#define SM3 "S_OK";

OBJLoader::OBJLoader() {}

OBJLoader::~OBJLoader() {}

CURRENT_VALUES OBJLoader::loadObj(const char* path)
{
	std::ifstream objFile;
	objFile.open(path);

	bool vFound = false, vtFound = false, nFound = false, fFound = false;

	if(objFile.is_open())
	{
		char type[128];

		while(!objFile.eof())
		{
			objFile >> type;

			if(strcmp(type, "v") == 0)
			{
				vFound = true;

				XMFLOAT3A vertex = {};

				/*fscanf: Which file are we reading from, read 3 (or 2) floats, where shall the data be stored*/
				objFile >> vertex.x >> vertex.y >> vertex.z;
				/*fscanf_s(
					dynamic_cast<FILE*>(&objFile), "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);*/
				m_currentValues.out_vertices.push_back(vertex);
				//std::cout << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
			}
			else if(strcmp(type, "vt") == 0)
			{
				vtFound = true;

				XMFLOAT2A uv = {};
				objFile >> uv.x >> uv.y;
				/*fscanf_s(dynamic_cast<FILE*>(&objFile), "%f %f\n", &uv.x, &uv.y);*/
				m_currentValues.out_uvs.push_back(uv);
			}
			else if(strcmp(type, "vn") == 0)
			{
				nFound = true;

				XMFLOAT3A normal = {};
				objFile >> normal.x >> normal.y >> normal.z;
				/*	fscanf_s(
					dynamic_cast<FILE*>(&objFile), "%f %f %f\n", &normal.x, &normal.y, &normal.z);*/
				m_currentValues.out_normals.push_back(normal);
				//std::cout << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
			}
			else if(strcmp(type, "f") == 0)
			{
				fFound = true;

				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
				char trash;

				objFile >> vertexIndex[0] >> trash >> uvIndex[0] >> trash >> normalIndex[0] >>
					vertexIndex[1] >> trash >> uvIndex[0] >> trash >> normalIndex[1] >>
					vertexIndex[2] >> trash >> uvIndex[0] >> trash >> normalIndex[2];

				/*objFile >> vertexIndex[0] >> trash >> trash >> normalIndex[0] >> vertexIndex[1] >>
					trash >> trash >> normalIndex[1] >> vertexIndex[2] >> trash >> trash >>
					normalIndex[2];*/

				/*objFile >> vertexIndex[0] >> vertexIndex[1] >>
					vertexIndex[2] ;*/
				/*This data corresponds to a face, stores the position of its verticed, its UV's and its normals.
				fscanf returns the amount of succesfully converted and assigned fields; meaning that if
				we do not get 9 matches, the data is faulty.*/

				/*	int matches = fscanf_s(reinterpret_cast<FILE*>(&objFile),
										   "%d/%d/%d %d/%d/%d %d/%d/%d\n",
										   &vertexIndex[0],
										   &uvIndex[0],
										   &normalIndex[0],
										   &vertexIndex[1],
										   &uvIndex[1],
										   &normalIndex[1],
										   &vertexIndex[2],
										   &uvIndex[2],
										   &normalIndex[2]);*/

				/*if(matches != 9)
				{
					m_currentValues.status = SM1;
					return m_currentValues;
				}
*/
				/*Store the indices, the 3 vertex indeces, the 2 UV indices and the
				3 normal indices. Store them in seperate arrays.*/
				m_currentValues.vertexIndices.push_back(vertexIndex[0] - 1);
				m_currentValues.vertexIndices.push_back(vertexIndex[1] - 1);
				m_currentValues.vertexIndices.push_back(vertexIndex[2] - 1);

				/*	m_currentValues.uvIndices.push_back(uvIndex[0]);
				m_currentValues.uvIndices.push_back(uvIndex[1]);
				m_currentValues.uvIndices.push_back(uvIndex[2]);*/

				m_currentValues.normalIndices.push_back(normalIndex[0] - 1);
				m_currentValues.normalIndices.push_back(normalIndex[1] - 1);
				m_currentValues.normalIndices.push_back(normalIndex[2] - 1);
			}
		}

		if(vFound && vtFound && nFound && fFound)
		{
			m_currentValues.status = SM3;
			return m_currentValues;
		}
		else
		{
			m_currentValues.status = SM2;
			return m_currentValues;
		}
	}
	else
	{
		m_currentValues.status = SM0;
		return m_currentValues;
	}
}
