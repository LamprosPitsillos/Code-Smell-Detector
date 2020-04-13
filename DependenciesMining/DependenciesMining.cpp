#include "DependenciesMining.h"
#include "Utilities.h"

#define CLASS_DECL "ClassDecl"
#define STRUCT_DECL "StructDecl"
#define FIELD_DECL "FieldDecl"
#define METHOD_DECL "MethodDecl"
#define METHOD_VAR_OR_ARG "MethodVarOrArg"
//#define MEMBER_EXPR "MemberExpr"

using namespace DependenciesMining;

StructuresTable DependenciesMining::structuresTable;

DeclarationMatcher ClassDeclMatcher = anyOf(cxxRecordDecl(isClass()).bind(CLASS_DECL), cxxRecordDecl(isStruct()).bind(STRUCT_DECL));
DeclarationMatcher FieldDeclMatcher = fieldDecl().bind(FIELD_DECL);
DeclarationMatcher MethodDeclMatcher = cxxMethodDecl().bind(METHOD_DECL);
DeclarationMatcher MethodVarMatcher = varDecl().bind(METHOD_VAR_OR_ARG);
//StatementMatcher MemberMatcher = memberExpr().bind(MEMBER_EXPR);

// Handle all the Classes and Structs and the Bases
void ClassDeclsCallback::run(const MatchFinder::MatchResult& result) {
	const CXXRecordDecl* d;
	Structure structure;
	if ((d = result.Nodes.getNodeAs<CXXRecordDecl>(CLASS_DECL))) {
		structure.SetStructureType(StructureType::Class);
	}
	else if ((d = result.Nodes.getNodeAs<CXXRecordDecl>(STRUCT_DECL))) {
		structure.SetStructureType(StructureType::Struct);
	}
	else {
		assert(0);
	}

	if (d->isInStdNamespace() || !d->hasDefinition()) {
		return;														// ignore std && declarations
	}
	// Templates
	if (d->getDescribedClassTemplate()) {									
		structure.SetStructureType(StructureType::TemplateDefinition);
	} 
	else if (d->getKind() == d->ClassTemplatePartialSpecialization) {
		structure.SetStructureType(StructureType::TemplatePartialSpecialization);

	} 
	else if (d->getKind() == d->ClassTemplateSpecialization) {
		if(d->getTemplateSpecializationKind() == 1)
			structure.SetStructureType(StructureType::TemplateInstatiationSpecialization);
		else 
			structure.SetStructureType(StructureType::TemplateFullSpecialization);
	}

	//std::cout << "Name: " << d->getNameAsString() << "\tQualifiedName: " << d->getQualifiedNameAsString() << "\nMy Name: " << GetFullStructureName(d) << "\n\n";
	
	structure.SetName(GetFullStructureName(d));

	// Templates
	if (d->getKind() == d->ClassTemplateSpecialization || d->getKind() == d->ClassTemplatePartialSpecialization) {
		// Template parent
		std::string parentName;
		if (structure.IsTemplateInstatiationSpecialization()) {
			auto* parent = d->getTemplateInstantiationPattern();
			parentName = GetFullStructureName(parent);
		}
		else {
			parentName = d->getQualifiedNameAsString();				// mporei kai na mhn xreiazetai na orisw parent mia kai to full specialization einai mia diaforetikh class (partial specialization einai san new template)
		}
		Structure* templateParent = structuresTable.Get(parentName);
		if (!templateParent)
			templateParent = structuresTable.Insert(parentName);
		structure.SetTemplateParent(templateParent);

		//Template Arguments
		auto* temp = (ClassTemplateSpecializationDecl*)d;
		for (unsigned i = 0; i < temp->getTemplateArgs().size(); ++i) {
			if (temp->getTemplateArgs()[i].getAsType()->isStructureOrClassType()) {
				std::string argName = GetFullStructureName(temp->getTemplateArgs()[i].getAsType()->getAsCXXRecordDecl());
				Structure* arg = structuresTable.Get(argName);
				structure.InsertTemplateSpecializationArguments(argName, arg);
			}
		}
	}
	
	// Namespace
	auto* enclosingNamespace = d->getEnclosingNamespaceContext();
	std::string fullEnclosingNamespace = "";
	while (enclosingNamespace->isNamespace()) {
		fullEnclosingNamespace = ((NamespaceDecl*)enclosingNamespace)->getNameAsString() + "::" + fullEnclosingNamespace;
		enclosingNamespace = enclosingNamespace->getParent()->getEnclosingNamespaceContext();
	}
	structure.SetEnclosingNamespace(fullEnclosingNamespace);

	// Bases
	for(auto it : d->bases()){
		auto* baseRecord = it.getType()->getAsCXXRecordDecl();	
		if (baseRecord == nullptr)										// otan base einai template or partial specialization Ignored
			continue;
		std::string baseName = GetFullStructureName(baseRecord);	
		Structure* base = structuresTable.Get(baseName);
		if (!base)
			base = structuresTable.Insert(baseName);
		structure.InsertBase(baseName, base);
	}

	// Friends 
	for (auto* it : d->friends()) {
		auto* type = it->getFriendType();
		if (type) {																							// Classes
			std::string parentName = GetFullStructureName(type->getType()->getAsCXXRecordDecl()) ;
			Structure* parentStructure = structuresTable.Get(parentName);
			if (!parentStructure) continue;								// templates
			structure.InsertFriend(parentName, parentStructure);
		}
		else {																								// Methods			
			auto* decl = it->getFriendDecl();
			if (decl->getKind() == it->CXXMethod) {
				std::string methodName = GetFullMethodName((CXXMethodDecl*)decl);
				auto* parentClass = ((CXXMethodDecl*)decl)->getParent();
				std::string parentName = GetFullStructureName(parentClass);
				Structure* parentStructure = structuresTable.Get(parentName);
				if (!parentStructure) continue;
				structure.InsertFriend(methodName, parentStructure);
			}
		}
	}

	// Members
	if (d->isCXXClassMember()) {
		const auto* parent = d->getParent();
		assert(parent);
		std::string parentName = GetFullStructureName((RecordDecl*)parent);
		if (parentName != structure.GetName()) {
			Structure* parentStructure = structuresTable.Get(parentName);
			auto* inst = d->getInstantiatedFromMemberClass();
			structure.SetNestedParent(parentStructure);
			parentStructure->InsertNestedClass(structure.GetName(), structuresTable.Insert(structure.GetName()));
		}
	}
	//std::cout << "\n";
	structuresTable.Insert(structure.GetName(), structure);
}

// Hanlde all the Fields in classes/structs
void FeildDeclsCallback::run(const MatchFinder::MatchResult& result) {
	if (const FieldDecl* d = result.Nodes.getNodeAs<FieldDecl>(FIELD_DECL)) {
		if (!isStructureOrStructurePointerType(d->getType()))
			return; 

		const RecordDecl* parent = d->getParent();
		if (parent->isClass() || parent->isStruct()) {
			std::string parentName = GetFullStructureName(parent);
			std::string typeName;
			if(d->getType()->isPointerType())
				typeName = GetFullStructureName(d->getType()->getPointeeType()->getAsCXXRecordDecl());
			else
				typeName = GetFullStructureName(d->getType()->getAsCXXRecordDecl());
			
			Structure* parentStructure = structuresTable.Get(parentName);
			Structure* typeStructure = structuresTable.Get(typeName);
			if (parentStructure->IsTemplateInstatiationSpecialization())		// insertion speciallization inherite its dependencies from the parent template
				return;
			if (!typeStructure)
				typeStructure = structuresTable.Insert(typeName);
			
			//llvm::outs() << "Field:  " << d->getName() << "\tQualified Name: " << d->getQualifiedNameAsString() << "\n\tParent: " << parentName << "   " << typeName << "\n";

			Definition field(d->getQualifiedNameAsString(), typeStructure);
			parentStructure->InsertField(d->getQualifiedNameAsString(), field);	
		} 
	}
}

// Handle all the Methods
void MethodDeclsCallback::run(const MatchFinder::MatchResult& result) {
	if (const CXXMethodDecl* d = result.Nodes.getNodeAs<CXXMethodDecl>(METHOD_DECL)) {
		const RecordDecl* parent = d->getParent();
		std::string parentName = GetFullStructureName(parent);
		Structure* parentStructure = structuresTable.Get(parentName);
		//llvm::outs() << "Method:  " << GetFullMethodName(d) << "\n\tParent: " << parentName << "\n\n";
		Method method(GetFullMethodName(d));
		if (d->getDeclKind() == d->CXXConstructor) {
			method.SetMethodType(MethodType::Constructor);
		}
		else if (d->getDeclKind() == d->CXXDestructor) {
			method.SetMethodType(MethodType::Destructor);
		}
		else if (d->getDeclKind() == d->ClassTemplate) {
			method.SetMethodType(MethodType::TemplateUserMethod);
		}
		else {
			method.SetMethodType(MethodType::UserMethod);
		}

		//Return
		auto returnType = d->getReturnType();
		if (isStructureOrStructurePointerType(returnType)) {
			std::string typeName;
			if (returnType->isPointerType())
				typeName = GetFullStructureName(returnType->getPointeeType()->getAsCXXRecordDecl());
			else
				typeName = GetFullStructureName(returnType->getAsCXXRecordDecl());
			Structure* typeStructure = structuresTable.Get(typeName);
			if (!typeStructure)
				typeStructure = structuresTable.Insert(typeName);
			method.SetReturnType(typeStructure);
		}

		currentMethod = parentStructure->InsertMethod(GetFullMethodName(d), method);

		// Body
		std::cout << GetFullMethodName(d) << "------------------\n";
		auto* body = d->getBody();
		FindMemberExprVisitor visitor; 
		visitor.TraverseStmt(body);
		std::cout << "------------------\n\n";
		
		currentMethod = nullptr;
	}
}

std::string MethodDeclsCallback::FindMemberExprVisitor::GetMemberExprAsString(MemberExpr* memberExpr) {
	auto* base = memberExpr->getBase();
	auto* decl = memberExpr->getMemberDecl();
	std::string exprString = decl->getNameAsString();
	std::string op;
	if (decl->getKind() == decl->CXXMethod)
		exprString += "()";
	while (true) {
		if (base->getStmtClass() == memberExpr->ImplicitCastExprClass) {
			/*// ---------------------------------------------------------------
			llvm::outs() << base->getStmtClassName() << " --- Type: ";
			if (base->getType()->isPointerType())
				std::cout << GetFullStructureName(base->getType()->getPointeeType()->getAsCXXRecordDecl()) << "\n";
			else {
				std::cout << GetFullStructureName(base->getType()->getAsCXXRecordDecl()) << "\n";
				std::cout << "WHAT??? NOT A POINTER\n\n";
			}
			// --------------------------------------------------------------- */
			base = (Expr*)*(((ImplicitCastExpr*)base)->child_begin());
		}
		if (base->getType()->isPointerType())
			op = "->";
		else
			op = ".";

		if (base->getStmtClass() == memberExpr->DeclRefExprClass) {				// apo var tou idiou tou method
			auto baseName = ((DeclRefExpr*)base)->getDecl()->getNameAsString();
			exprString = baseName + op + exprString;
			break;
		}
		else if (base->getStmtClass() == memberExpr->MemberExprClass) {	
			auto baseName = ((MemberExpr*)base)->getMemberDecl()->getNameAsString();
			if (((MemberExpr*)base)->getMemberDecl()->getKind() == decl->CXXMethod)
				exprString = baseName + "()" + op + exprString;
			else 
				exprString = baseName + op + exprString;

			base = ((MemberExpr*)base)->getBase();
		}
		else if (base->getStmtClass() == memberExpr->CXXThisExprClass) {
			break;
		}
		else {
			llvm::outs() << base->getStmtClassName() << " ???????????\n";
			assert(0);
		}
	}
	return exprString;
}

bool MethodDeclsCallback::FindMemberExprVisitor::VisitMemberExpr(MemberExpr* memberExpr) {
	auto* decl = memberExpr->getMemberDecl();
	auto type = decl->getType();
	auto* base = memberExpr->getBase();
	bool isMethod = false;
	if (base->getStmtClass() == memberExpr->CXXThisExprClass) { // ignore class' fields
		std::cout << "~~~~~~~~~~~~~~~this\n\n";
		return true;
	}

	if (!isStructureOrStructurePointerType(type)) {
		if (decl->getKind() == decl->CXXMethod) {
			CXXMethodDecl* methodDecl = (CXXMethodDecl*)decl;
			type = methodDecl->getReturnType();
			isMethod = true;
			if (!isStructureOrStructurePointerType(type)) {
				std::cout << "~~~~~~~~~~~~~~~method\n\n";
				return true;
			}
		}
		else
			return true;
	}
	std::string typeName;
	if (type->isPointerType())
		typeName = GetFullStructureName(type->getPointeeType()->getAsCXXRecordDecl());
	else
		typeName = GetFullStructureName(type->getAsCXXRecordDecl());
	Structure* typeStructure = structuresTable.Get(typeName);
	if (!typeStructure)
		typeStructure = structuresTable.Insert(typeName);

	llvm::outs() << "Decl Type: " << typeName << "\n";

	std::string exprString = GetMemberExprAsString(memberExpr);
	std::cout << "Member Expr:  " << exprString << "\n"; 
	
	Method::MemberExpr methodMemberExpr(decl->getNameAsString(), exprString, isMethod);
	MethodDeclsCallback::currentMethod->InsertMemberExpr(typeName, typeStructure, methodMemberExpr);
	
	/*if (base->getStmtClass() == memberExpr->ImplicitCastExprClass) {
		// ---------------------------------------------------------------
		llvm::outs() << base->getStmtClassName() << " --- Type: ";
		if (base->getType()->isPointerType())
			std::cout << GetFullStructureName(base->getType()->getPointeeType()->getAsCXXRecordDecl()) << "\n";
		else {
			std::cout << GetFullStructureName(base->getType()->getAsCXXRecordDecl()) << "\n";
			std::cout << "WHAT??? NOT A POINTER\n\n";
		}
		// ---------------------------------------------------------------

		base = (Expr*)*(((ImplicitCastExpr*)base)->child_begin());
	}
	
	if (base->getStmtClass() == memberExpr->DeclRefExprClass) {
		// apo var tou idiou tou method
		auto baseName = ((DeclRefExpr*)base)->getDecl()->getNameAsString();
		std::cout << "DeclRef - BaseName: " << baseName << "\n";
	}
	else if (base->getStmtClass() == memberExpr->MemberExprClass) {
		// apo field ths class
		auto baseName = ((MemberExpr*)base)->getMemberDecl()->getNameAsString();
		std::cout << "MemberExpr - BaseName: " << baseName << "\n";

	}
	else if (base->getStmtClass() == memberExpr->CXXThisExprClass) {
		// apo this
		std::cout << "~~~~~~~~~~~~~~~this\n\n";
		return true;
	}
	else {
		llvm::outs() << base->getStmtClassName() << " ???????????\n";
	}*/
	
	llvm::outs() << "Kind: " << decl->getDeclKindName() << "\n";
	llvm::outs() << "Name: " << decl->getNameAsString() << "\n";
	
	std::cout << "~~~~~~~~~~~~~~~\n\n";
	return true;
}

// Handle Method's Vars and Args
void MethodVarsCallback::run(const MatchFinder::MatchResult& result) {
	if (const VarDecl* d = result.Nodes.getNodeAs<VarDecl>(METHOD_VAR_OR_ARG)) {
		auto* parentMethod = d->getParentFunctionOrMethod();
		if (d->isLocalVarDeclOrParm() && parentMethod && parentMethod->getDeclKind() == d->CXXMethod) {	// including params
		//if(d->isFunctionOrMethodVarDecl() && parentMethod->getDeclKind() == d->CXXMethod){		// excluding params	- d->isFunctionOrMethodVarDecl()-> like isLocalVarDecl() but excludes variables declared in blocks?.		
			if (!isStructureOrStructurePointerType(d->getType()))
				return;

			auto* parentClass = (CXXRecordDecl*)parentMethod->getParent();
			auto parentClassName = GetFullStructureName(parentClass);
			auto parentMethodName = GetFullMethodName((CXXMethodDecl*)parentMethod);
			Structure* parentStructure = structuresTable.Get(parentClassName);
			Method* parentMethod = parentStructure->GetMethod(parentMethodName);
			std::string typeName;
			if (d->getType()->isPointerType())
				typeName = GetFullStructureName(d->getType()->getPointeeType()->getAsCXXRecordDecl());
			else
				typeName = GetFullStructureName(d->getType()->getAsCXXRecordDecl());
			Structure* typeStructure = structuresTable.Get(typeName);
			if (!typeStructure)
				typeStructure = structuresTable.Insert(typeName);
			Definition def(d->getQualifiedNameAsString(), typeStructure);
			if (d->isLocalVarDecl()) {
				parentMethod->InsertDefinition(d->getQualifiedNameAsString(), def);
			}
			else {
				parentMethod->InsertArg(d->getQualifiedNameAsString(), def);
			}
		}
	}
}
/*
	Clang Tool Creation
*/
static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nA help message for this specific tool can be added afterwards..\n");

int DependenciesMining::CreateClangTool(int argc, const char** argv, std::vector<std::string> srcs) {
	CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
	ClangTool Tool(OptionsParser.getCompilations(), srcs);
	
	ClassDeclsCallback classCallback;
	FeildDeclsCallback fieldCallback;
	MethodDeclsCallback methodCallback;
	MethodVarsCallback methodVarCallback;
	MatchFinder Finder;
	Finder.addMatcher(ClassDeclMatcher, &classCallback);
	Finder.addMatcher(FieldDeclMatcher, &fieldCallback); 
	Finder.addMatcher(MethodDeclMatcher, &methodCallback);
	Finder.addMatcher(MethodVarMatcher, &methodVarCallback);
	int result = Tool.run(newFrontendActionFactory(&Finder).get());
	return result;
}
