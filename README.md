# µCSV

## Setup

```cmake
FetchContent_Declare(
	uCSV
	GIT_REPOSITORY https://github.com/fytch/uCSV.git
)
FetchContent_MakeAvailable(uCSV)
target_link_libraries(YOURPROGRAM uCSV)
```

## Example

```cpp
#include <uCSV.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <deque>

namespace foo
{
	struct vec3
	{
		double x, y, z;
	};

	void deserialize(uCSV::Deserializer& data, vec3& vec)
	{
		using uCSV::deserialize;
		deserialize(data, vec.x);
		deserialize(data, vec.y);
		deserialize(data, vec.z);
	}
}

int main()
{
	// This file is supposed to be structured as follows:
	// 0,0,1
	// 0.2,0.3,-0.4
	// ...
	std::ifstream file1("vecs.csv", std::ifstream::binary);
	if(!file1.is_open())
	{
		std::cerr << "vecs.csv couldn't be opened\n";
		return 1;
	}
	uCSV::Reader reader1(file1, uCSV::ErrorLog(std::cerr), uCSV::Delimiter<','>{}, uCSV::ignoreHeader);
	std::deque<foo::vec3> vecs;
	reader1.fetchAll(std::back_inserter(vecs));

	// This file is supposed to be structured as follows:
	// id,px,py,pz,vx,vy,vz
	// 0,2,2,3,1,0,0
	// ...
	std::ifstream file2("particles.csv", std::ifstream::binary);
	if(!file2.is_open())
	{
		std::cerr << "particles.csv couldn't be opened\n";
		return 1;
	}
	uCSV::Reader reader2(file2, uCSV::ErrorLog(std::cerr), uCSV::readHeader);
	std::deque<int> ids;
	std::deque<foo::vec3> positions;
	std::deque<foo::vec3> velocities;
	while(!reader2.done())
	{
		int id;
		foo::vec3 p, v;
		auto des = reader2.fetch();
		uCSV::deserializeMany(*des, id, p, v);
		ids.push_back(id);
		positions.emplace_back(std::move(p));
		velocities.emplace_back(std::move(v));
	}
}
```
