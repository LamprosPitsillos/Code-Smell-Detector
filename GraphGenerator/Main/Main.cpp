#include "DependenciesMining.h"
#include "Gui.h"
#include "SourceLoader.h"
#include "ImportST.h"
#include "LoadGlobalCache.h"
#include "Graph.h"
#include "GraphToJson.h"
#include "GraphGeneration.h"
#include "json/writer.h"
#include <iostream>
#include <cstdlib>

using namespace dependenciesMining;
using namespace sourceLoader;
using namespace incremental;
using namespace graph;
using namespace graphGeneration;
using namespace graphToJson;

using FilePaths = std::vector<std::string>;

// NOTE: Make sure that either there is no .json out file, or the .json out file was generated by mining with the INCREMENTAL_GENERATION flag enabled.

namespace {

	void PrintMainArgInfo(std::ostream& os = std::cerr) {
		os << "MAIN ARGUMENTS:\n\n";
		os << "argv[1]: \"--src\" to mine whole directory with sources (argv[2]: directory/with/sources)\n";
		os << "argv[1]: \"--cmp-db\" to use compilation database (argv[2]: path/to/compile_commands.json)\n";
		os << "argv[3]: (file path) path/to/ignoredFilePaths\n";
		os << "argv[4]: (file path) path/to/ignoredNamespaces\n";
		os << "argv[5]: (file path) path/to/ST-output\n";
	}

	void PrintClangToolInfo(std::string& errorMsg, std::ostream& os = std::cerr) {
		os << "Clang Tool creation failed\n";
		os << errorMsg << '\n';
	}

	void PrintMiningResult(int res, std::ostream& os = std::cout) {
		if (res == 0) 
			os << "\nCOMPILATION FINISHED\n";
		else if (res == 1)
			os << "\nCOMPILATION FAILED\n";
		else if (res == 2) 
			os << "\nCOMPILATION FINISHED (with skipped files)\n";
		else
			assert(false && "See: `int clang::tooling::ClangTool::run(clang::tooling::ToolAction *Action)`");
	}

} // namespace

namespace {

	void SerializeDependencies(const Json::Value& graph, Json::Value& ST) {
		const Json::Value& all_dependencies = graph["edges"];
		auto& st_dependencies = ST["dependencies"];

		for (const auto& dependencies : all_dependencies) {
			Json::Value dependency_pack;
			dependency_pack["types"] = dependencies["dependencies"];
			dependency_pack["from"] = dependencies["from"];
			dependency_pack["to"] = dependencies["to"];

			st_dependencies.append(dependency_pack);
		}
	}

	void SerializeFilePaths(Json::Value& ST, const FilePaths& srcs, const FilePaths& headers) {
		for (const auto& path : srcs) 
			ST["sources"].append(path);
		for (const auto& path : headers) 
			ST["headers"].append(path);
	}

	void ProduceJsonOutput(const Graph& dependencyGraph, const SymbolTable& exportedTable, const FilePaths& srcs, const FilePaths& headers, const char* outputPath) {
		assert(outputPath);

		Json::Value jsonST;
		exportedTable.AddJsonSymbolTable(jsonST["structures"]);

		const auto jsonGraph = GetJson(dependencyGraph);

		SerializeDependencies(jsonGraph, jsonST);
		SerializeFilePaths(jsonST, srcs, headers);

		std::ofstream jsonSTFile{outputPath};
		jsonSTFile << jsonST;
		std::cout << "\nGRAPH GENERATED\n";

		assert(jsonSTFile.good());
		assert(std::filesystem::exists(outputPath));
	}

	void ExportDependencies(ClangTool& tool,const SymbolTable& exportedTable, const char* outputPath) {
		assert(outputPath);

		const auto dependenciesGraph = GenerateDependenciesGraph(exportedTable);

		FilePaths srcs, headers;
		GetMinedFiles(tool, srcs, headers);

		ProduceJsonOutput(dependenciesGraph, structuresTable, srcs, headers, outputPath);
	}

} // namespace

int main(int argc, char* argv[]) {
	if (argc != 6) {
		PrintMainArgInfo();
		return EXIT_FAILURE;
	}

	constexpr std::string_view srcOption = "--src";
	constexpr std::string_view databaseOption = "--cmp-db";

	const auto* compilationOption = argv[1];
	const auto* inputPath = argv[2];
	const auto* ignoredFilesPath = argv[3];
	const auto* ignoredNamespacesPath = argv[4];
	const auto* outputPath = argv[5];

	std::unique_ptr<ClangTool> clangTool;
	std::string errorMsg;

	if (compilationOption == srcOption) {
		clangTool = CreateClangTool(SourceLoader{ inputPath }.GetSources());

	} else if (compilationOption == databaseOption) {
		clangTool = CreateClangTool(inputPath, errorMsg);

	} else {
		PrintMainArgInfo();
		return EXIT_FAILURE;
	}

	if (!clangTool) {
		PrintClangToolInfo(errorMsg);
		return EXIT_FAILURE;
	}

#ifdef INCREMENTAL_GENERATION
	ImportStashedST(outputPath, structuresTable);

	LoadGlobalCache(structuresTable, cache);
#endif

	SetIgnoredRegions(ignoredFilesPath, ignoredNamespacesPath);

	std::cout << "\n-------------------------------------------------------------------------------------\n\n";
	int miningRes;

#ifdef GUI
	wxEntryStart(argc, argv);

	wxGetApp().SetMax(clangTool->getSourcePaths().size());
	wxGetApp().SetOnCancel([&clangTool, outputPath]() {
		ExportDependencies(*clangTool, structuresTable, outputPath);

		std::exit(EXIT_SUCCESS);
	});
	
 	wxTheApp->CallOnInit();

	miningRes = MineArchitecture(*clangTool);

	wxGetApp().Finished();
  	//wxTheApp->OnRun(); 

	PrintMiningResult(miningRes);

	ExportDependencies(*clangTool, structuresTable, outputPath);

#else
	miningRes = MineArchitecture(*clangTool);
	
	PrintMiningResult(miningRes);

	ExportDependencies(*clangTool, structuresTable, outputPath);
#endif

	return EXIT_SUCCESS;
}