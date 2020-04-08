#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>

namespace DependenciesMining {

	class Structure;

	enum class StructureType {
		Undefined,
		Class,
		Struct,
		TemplateDefinition,
		TemplateFullSpecialization,
		TemplateInstatiationSpecialization,
		TemplatePartialSpecialization
	};

	class Definition {
	private:
		std::string name;
		Structure* type;

	public:
		Definition() = default;
		Definition(const std::string& name, Structure* type) : name(name), type(type) {};
		std::string GetName() const;
		const Structure* GetType() const;
	};


	class Method {
	private:
		std::string name;
		Structure* returnType = nullptr;
		std::unordered_map<std::string, Definition> arguments;
		std::unordered_map<std::string, Definition> definitions;
		std::unordered_map<std::string, Structure*> memberExprs;

	public:
		Method() = default;
		Method(std::string name) : name(name) {};
		std::string GetName() const;
		std::unordered_map<std::string, Definition>& GetArguments() ;
		std::unordered_map<std::string, Definition>& GetDefinitions();
		std::unordered_map<std::string, Structure*>& GetMemberExprs();
		Structure* GetReturnType();

		void SetReturnType(Structure* structure);
		void InsertArg(const std::string& name, Definition& definition);
		void InsertDefinition(const std::string& name, Definition& definition);
		void InsertMemberExpr(const std::string& name, Structure* structure);
	};

	class Template {
	private:
		Structure* parent = nullptr; 
		std::unordered_map<std::string, Structure*> specializationArguments;
		
	public:
		Template() = default;
		void SetParent(Structure* structure);
		void InsertSpecializationArguments(const std::string& name, Structure* structure);
	};

	class Structure {
	private:
		std::string name;
		std::string enclosingNamespace = "";
		StructureType structureType = StructureType::Undefined;
		Template templateInfo;
		Structure* nestedParent = nullptr;
		std::unordered_map<std::string, Method> methods;
		std::unordered_map<std::string, Definition> fields;
		std::unordered_map<std::string, Structure*> bases;
		std::unordered_map<std::string, Structure*> contains;
		std::unordered_map<std::string, Structure*> friends;		// About Structures: Key->structureName, Value->Structure*
																	// About Methods: Key->methodName, Value->Structure*
				
	public:
		Structure(Structure& structure);
		Structure() = default;
		Structure(std::string name, std::string enclosingNamespace = "", StructureType structureType = StructureType::Undefined)
			: name(name), enclosingNamespace(enclosingNamespace), structureType(structureType) {};
		std::string GetName() const;
		std::string GetEnclosingNamespace() const;
		StructureType GetStructureType() const;
		Structure* GetNestedParent() const;
		Method* GetMethod(const std::string& name); 
		std::unordered_map<std::string, Method>& GetMethods();
		std::unordered_map<std::string, Definition>& GetFields();
		std::unordered_map<std::string, Structure*>& GetBases();
		std::unordered_map<std::string, Structure*>& GetContains();
		std::unordered_map<std::string, Structure*>& GetFriends();
		
		void SetName(const std::string& name);
		void SetEnclosingNamespace(const std::string& enclosingNamespace);
		void SetStructureType(StructureType structureType);
		void SetTemplateInfo(Template temp);
		void SetTemplateParent(Structure* structure);
		void SetNestedParent(Structure* structure);

		Method* InsertMethod(const std::string& name, Method& method);
		void InsertField(const std::string& name, Definition& definition);
		void InsertBase(const std::string& name, Structure* structure);
		void InsertNestedClass(const std::string& name, Structure* structure);
		void InsertFriend(const std::string& name, Structure* structure);
		void InsertTemplateSpecializationArguments(const std::string& name, Structure* structure);

		bool IsTemplateDefinition();
		bool IsTemplateFullSpecialization();
		bool IsTemplateInstatiationSpecialization();
		bool IsTemplatePartialSpecialization();
		bool IsTemplate(); 
		bool IsNestedClass();
	};


	class StructuresTable {
	private:
		std::unordered_map<std::string, Structure> table;
	public: 
		std::unordered_map<std::string, Structure>& GetSymbolTable(); 
		Structure* Insert(const std::string& name);
		Structure* Insert(const std::string& name, Structure& structure);
		Structure* Get(const std::string& name);

		void Print();
	};
}