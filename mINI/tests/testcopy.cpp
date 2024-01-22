#include <iostream>
#include "lest.hpp"
#include "ini.h"

bool compareData(mINI::INIStructure a, mINI::INIStructure b)
{
	for (auto const& it : a)
	{
		auto const& section = std::get<0>(it);
		auto const& collection = std::get<1>(it);
		if (collection.size() != b[section].size()) {
			return false;
		}
		for (auto const& it2 : collection)
		{
			auto const& key = std::get<0>(it2);
			auto const& value = std::get<1>(it2);
			if (value != b[section][key]) {
				return false;
			}
		}
		std::cout << std::endl;
	}
	return a.size() == b.size();
}

void outputData(mINI::INIStructure const& ini)
{
	for (auto const& it : ini)
	{
		auto const& section = std::get<0>(it);
		auto const& collection = std::get<1>(it);
		std::cout << "[" << section << "]" << std::endl;
		for (auto const& it2 : collection)
		{
			auto const& key = std::get<0>(it2);
			auto const& value = std::get<1>(it2);
			std::cout << key << "=" << value << std::endl;
		}
		std::cout << std::endl;
	}
}

const lest::test mINI_tests[] = {
	CASE("TEST: Copy semantics")
	{
		mINI::INIStructure iniA;
		
		iniA["a"].set({
			{ "x", "1", "//comment1" },
			{ "y", "2", "//comment2" },
			{ "z", "3", "//comment3" }
		});
		
		iniA["b"].set({
			{ "q", "100", "//comment1" },
			{ "w", "100", "//comment2" },
			{ "e", "100", "//comment3" }
		});
		
		mINI::INIStructure iniB(iniA);
		EXPECT(compareData(iniA, iniB));
		
		mINI::INIStructure iniC = iniA;
		EXPECT(compareData(iniA, iniC));
	}
};

int main(int argc, char** argv)
{
	// run tests
	if (int failures = lest::run(mINI_tests, argc, argv))
	{
		return failures;
	}
	return std::cout << std::endl << "All tests passed!" << std::endl, EXIT_SUCCESS;
}