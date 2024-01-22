#include "IniReader.h"
#include "lest.hpp"

using T_LineData = std::vector<std::string>;
using T_INIFileData = std::tuple<std::string, T_LineData, T_LineData>;

bool writeTestFile(T_INIFileData const& testData)
{
	WCHAR buffer[MAX_PATH];
	HMODULE hm = NULL;
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&writeTestFile, &hm);
	GetModuleFileNameW(hm, buffer, ARRAYSIZE(buffer));
	std::filesystem::path modulePath(buffer);

	std::string const& filename = std::get<0>(testData);
	T_LineData const& lines = std::get<1>(testData);
	std::ofstream fileWriteStream(modulePath.parent_path() / filename);
	if (fileWriteStream.is_open())
	{
		if (lines.size())
		{
			auto it = lines.begin();
			for (;;)
			{
				fileWriteStream << *it;
				if (++it == lines.end())
				{
					break;
				}
				fileWriteStream << std::endl;
			}
		}
		return true;
	}
	return false;
}

T_INIFileData testDataBasic{
	// filename
	"data01.ini",
	// original data
	{
		";some comment",
		"[some section]",
		"some key=1  //comment1",
		"FLT=2.55 # comment2",
		"BOOLEAN = True     # comment2",
		"STRING=text # comment2",
		"Find/Replace=(X=361,Y=248,XL=458,YL=193)",
	},
	// expected result
	{
		""
	}
};

const lest::test mINI_tests[] = {
	CASE("TEST: IniReader")
	{
		CIniReader iniReader("data01.ini");
		
		auto i = iniReader.ReadInteger("some section", "some key", -1);
		EXPECT(i == 1);
		
		auto i2 = iniReader.ReadInteger("some section2", "some key2", -1);
		EXPECT(i2 == -1);
		
		auto f = iniReader.ReadFloat("some section", "FLT", -1.0f);
		EXPECT(fabs(f - 2.55f) <= FLT_EPSILON);
		
		auto b = iniReader.ReadBoolean("some section", "BOOLEAN", true);
		EXPECT(b == true);
		
		auto s = iniReader.ReadString("some section", "STRING", "fail");
		EXPECT(s == "text");
	
		auto s2 = iniReader.ReadString("some section", "Find/Replace", "fail");
		EXPECT(s2 == "(X=361,Y=248,XL=458,YL=193)");
	},

	CASE("TEST: IniWriter")
	{
		auto compare_files = [](const std::filesystem::path& filename1, const std::filesystem::path& filename2) -> bool
		{
			std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); //open file at the end
			std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); //open file at the end
			const std::ifstream::pos_type fileSize = file1.tellg();

			if (fileSize != file2.tellg()) {
				return false; //different file size
			}

			file1.seekg(0); //rewind
			file2.seekg(0); //rewind

			std::istreambuf_iterator<char> begin1(file1);
			std::istreambuf_iterator<char> begin2(file2);

			return std::equal(begin1,std::istreambuf_iterator<char>(),begin2); //Second argument is end-of-range iterator
		};

		CIniReader iniReader("SplinterCell.in");
		CIniReader iniWriter("SplinterCell.in");
		iniWriter.SetNewIniPathForSave("SplinterCell.ini");

		iniWriter.WriteInteger("D3DDrv.D3DRenderDevice", "ForceShadowMode", 1);
		iniWriter.WriteString("D3DDrv.D3DRenderDevice", "FullScreenVideo", "True");
		
		EXPECT(compare_files(iniReader.GetIniPath(), iniWriter.GetIniPath()) == true);
	}
};

int main(int argc, char** argv)
{
	writeTestFile(testDataBasic);
	// run tests
	if (int failures = lest::run(mINI_tests, argc, argv))
	{
		return failures;
	}
	return std::cout << std::endl << "All tests passed!" << std::endl, EXIT_SUCCESS;
}