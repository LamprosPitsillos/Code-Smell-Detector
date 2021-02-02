#pragma warning(disable : 4996)
#pragma warning(disable : 4146)
#include <iostream>
#include <fstream>
#include "SourceLoader.h"
#include "DependenciesMining.h"
#include "GraphGeneration.h"
#include "GraphToJson.h"

int main(int argc, const char** argv) {
	const char* cmpDBPath = argv[1];
	const char* ignoredFilePaths = (argc >= 3) ? argv[2] : "";
	const char* ignoredNamespaces = (argc >= 4) ? argv[3] : "";
	
	std::string fullPath = std::string(__FILE__);
	std::size_t found = fullPath.find_last_of("/\\");
	std::string jsonPath = (argc >= 5) ? argv[4] : fullPath.substr(0, found + 1) + "../../GraphVisualizer/Graph/graph.json";

	/*sourceLoader::SourceLoader srcLoader(path);
	srcLoader.LoadSources();
	std::vector<std::string> srcs = srcLoader.GetSources();*/
	
	//std::vector<std::string> srcs;
	/*srcs.push_back(path + "\\classes_simple.cpp");			
	srcs.push_back(path + "\\fields.cpp");					
	srcs.push_back(path + "\\friends.cpp");					
	srcs.push_back(path + "\\member_classes.cpp");			
	srcs.push_back(path + "\\methods_args_vars.cpp");		
	srcs.push_back(path + "\\methods.cpp");					
	srcs.push_back(path + "\\namespaces.cpp");			
	//srcs.push_back(path + "\\objects_used_on_methods.cpp");	
	srcs.push_back(path + "\\template_methods.cpp");		
	srcs.push_back(path + "\\template_types.cpp");			
	srcs.push_back(path + "\\templates.cpp");				

	srcs.push_back(path + "\\test0.cpp");					
	srcs.push_back(path + "\\include.h");						
	srcs.push_back(path + "\\include2.h");*/
				
	std::cout << "\n-------------------------------------------------------------------------------------\n\n";
	int result = dependenciesMining::CreateClangTool(cmpDBPath, ignoredFilePaths, ignoredNamespaces);
	graph::Graph graph = graphGeneration::GenetareDependenciesGraph(dependenciesMining::structuresTable);
	std::string json = graphToJson::GetJson(graph);

	std::ofstream jsonFile;

	jsonFile.open(jsonPath);
	jsonFile << json;
 	jsonFile.close();
	std::cout << "\n-------------------------------------------------------------------------------------\n\n";
}