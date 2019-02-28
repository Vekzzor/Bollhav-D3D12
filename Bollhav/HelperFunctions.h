#pragma once

namespace Resize 
{
	template<typename T> 
	void ResizeArray(T* copySource, UINT oldSize, UINT newSize)
	{
		if(copySource != nullptr)
		{
			T* newArray = new T[newSize];

			for(int i = 0; i < oldSize; i++)
			{
				newArray[i] = copySource[i];
			}

			delete copySource;
			copySource = newArray; 
		}
		else
		{
			std::cout << "ARRAY COULD NOT BE RESIZED" << std::endl; 
			exit(0); 
		}
	}
} 