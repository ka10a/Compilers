#include "TableBuilder.h"
#include "Symbol.h"
#include <cassert>
#include <iostream>

TableBuilder::TableBuilder(): table(new Table), curClass(nullptr), curMethod(nullptr), curVar(nullptr) {}

bool TableBuilder::hasClass(Symbol* className)
{
    return table->classes.find(className) != table->classes.end();
}

void TableBuilder::visit(const Goal* n) {
	n->main_class->Accept(this);
	for (auto& classDecl: *(n->class_declarations)) {
		classDecl->Accept(this);
	}

    for (auto& classDecl: *n->class_declarations) {
        curClass = table->classes[dynamic_cast<Identifier*>(dynamic_cast<ClassDeclaration*>(classDecl.get())->class_name.get())->name];
        for (auto& cl: table->classes) {
            if (cl.second->parent != nullptr && cl.second->parent == curClass->name) {
                cl.second->parentInfo = curClass;
                for (auto& var: cl.second->vars) {
                    if (curClass->HasField(var.second->symbol)) {
                        // std::string error = string_format(
                        //         "Variable %s was already declared as field of class %s.\n",
                        //         var.second->symbol->getString().c_str(),
                        //         cl.second->name->getString().c_str());
                        std::cout << "error: variable already was declared\n";
                        errors.push_back("error: variable already was declared\n");
                    }
                }

                for (auto& met: cl.second->methods) {
                    if (curClass->HasMethod(met.second->name)) {
                        auto method = curClass->GetMethod(met.second->name);
                        // TODO : Add type checking
                        if (method->type->type != met.second->type->type ||
                            method->args.size() != met.second->args.size()) {
                            // std::string error = string_format(
                            //         "Method %s was already declared in class %s.\n",
                            //         met.second->name->getString().c_str(),
                            //         curClass->name->getString().c_str());
                            std::cout << "error: method already was declared -- 1\n";
                            errors.push_back("error: method already was declared -- 1\n");
                        } else {
                            for (auto arg1 = method->args.begin(), arg2 = met.second->args.begin();
                                arg1 != method->args.end();
                                arg1++, arg2++) {
                                if (arg1->second->type->type != arg2->second->type->type) {
                                    std::cout << "error: method already was declared -- 2\n";
                                    errors.push_back("error: method already was declared -- 2\n");
                                    break;
                                }
                            }
                        }
                    }
                    for (auto& arg: met.second->args) {
                        if (auto classVal = std::get_if<ClassType>(&(arg.second->type->type))) {
                            if (table->classes.find(dynamic_cast<Identifier*>(classVal->name)->name) == table->classes.end()) {
                                std::cout << "error: type of arg was not declared yet\n";
                                errors.push_back("error: type of arg was not declared yet\n");
                            }
                        }
                    }
                    if (auto classVal = std::get_if<ClassType>(&(met.second->type->type))) {
                        if (table->classes.find(dynamic_cast<Identifier*>(classVal->name)->name) == table->classes.end()) {
                            std::cout << "error: type of arg was not declared yet\n";
                            errors.push_back("error: type of arg was not declared yet\n");
                        }
                    }
                }
            }
        }
        curClass = nullptr;
    }

    for (auto& classDecl: *n->class_declarations) {
        curClass = table->classes[dynamic_cast<Identifier*>(dynamic_cast<ClassDeclaration*>(classDecl.get())->class_name.get())->name];
        if (curClass->parent != nullptr && (curClass->parentInfo == nullptr || curClass->parentInfo->parent == curClass->name)) {
            // std::string error = string_format( "Class %s extends at class %s which wasn't declared\n",
            //                                    curClass->name->getString().c_str(),
            //                                    curClass->parent->getString().c_str());
            std::cout << "error: class extension\n";
            errors.push_back("error: class extension\n");
        }

        for (auto& met: curClass->methods) {
            for (auto& arg: met.second->args) {
                if (auto classVal = std::get_if<ClassType>(&(arg.second->type->type))) {
                    if (table->classes.find(dynamic_cast<Identifier*>(classVal->name)->name) == table->classes.end()) {
                        std::cout << "error: type of arg was not declared yet\n";
                        errors.push_back("error: type of arg was not declared yet\n");
                    }
                }
            }
            if (auto classVal = std::get_if<ClassType>(&(met.second->type->type))) {
                if (table->classes.find(dynamic_cast<Identifier*>(classVal->name)->name) == table->classes.end()) {
                    std::cout << "error: type of arg was not declared yet\n";
                    errors.push_back("error: type of arg was not declared yet\n");
                }
            }
        }
        curClass = nullptr;
    }
}

void TableBuilder::visit(const MainClass* n) {
	curClass = new ClassInfo(dynamic_cast<Identifier*>(n->class_name.get())->name, nullptr);

    if (hasClass(curClass->name)) {
        // std::string error = string_format("Class %s was already declared\n",
        //                                    curClass->name->getString().c_str());
        std::cout << "error: class already was declared -- 1\n";
        errors.push_back("error: class already was declared\n");
    }

    table->classes[curClass->name] = curClass;

    curMethod = new MethodInfo(nullptr, InternTable::getIntern("main"));
    curVar = new VariableInfo(nullptr, dynamic_cast<Identifier*>(n->arg.get())->name);
    curMethod->args[curVar->symbol] = curVar;
    curClass->methods[curMethod->name] = curMethod;

	// method = new MethodInfo(getIndent("main"), nullptr);
	// var = new VariableInfo(nullptr, n->arg->name)
	// method->args[n->arg->name] = var;
	// cl->methods[method->name] = method;
	// table->classes[cl->name] = cl;
	// var = 0;
	// method = 0;
	curClass = nullptr;
    curMethod = nullptr;
    curVar = nullptr;
}

void TableBuilder::visit(const ClassDeclaration* n) {
	curClass = new ClassInfo(dynamic_cast<Identifier*>(n->class_name.get())->name,
        n->extends_class_name == nullptr ? nullptr : dynamic_cast<Identifier*>(n->extends_class_name.get())->name);

    if (hasClass(curClass->name)) {
        // std::string error = string_format("Class %s was already declared\n",
        //                                    curClass->name->getString().c_str());
        std::cout << "error: class already was declared -- 2\n";
        errors.push_back("error: class already was declared.\n");
    }
	table->classes[curClass->name] = curClass;
	for (auto& varDeclaration: *(n->vars)) {
        varDeclaration->Accept(this);
    }
    for (auto& methodDeclaration: *(n->methods)) {
        methodDeclaration->Accept(this);
    }
    curClass = nullptr;
}

void TableBuilder::visit(const VarDeclaration* n) {
    // Type problems
	curVar = new VariableInfo(dynamic_cast<Type*>(n->type.get()), dynamic_cast<Identifier*>(n->name.get())->name);
	if (curMethod != nullptr) {
        if (curMethod->locals.find(curVar->symbol) != curMethod->locals.end()) {
            // std::string error = string_format(
            //         "Variable %s was already declared as local variable of method %s.\n",
            //         curVar->symbol->getString().c_str(),
            //         curMethod->name->getString().c_str());
            std::cout << "error: var already was declared as local var.\n";
            errors.push_back("error: var already was declared as local var.\n");
        }
        curMethod->locals[curVar->symbol] = curVar;
	} else {
		if (curClass->HasField(curVar->symbol)) {
            // std::string error = string_format(
            //         "Variable %s was already declared as field of class %s.\n",
            //         curVar->symbol->getString().c_str(),
            //         curClass->name->getString().c_str());
            std::cout << "error: var already was declared as class field.\n";
            errors.push_back("error: var already was declared as class field.\n");
        }
        curClass->vars[curVar->symbol] = curVar;
	}
	curVar = nullptr;
}

void TableBuilder::visit(const MethodDeclaration* n) {
    // Type problems
	curMethod = new MethodInfo(dynamic_cast<Type*>(n->return_type.get()), dynamic_cast<Identifier*>(n->name.get())->name);
	if (curClass->HasMethod(curMethod->name)) {
        auto method = curClass->GetMethod(curMethod->name);
        if (method->args.size() != n->args->size()) {
            // std::string error = string_format(
            //         "Method %s was already declared in class %s.\n",
            //         curMethod->name->getString().c_str(),
            //         curClass->name->getString().c_str());
            std::cout << "error: Method was already declared.\n";
            errors.push_back("error: Method was already declared.\n");
        }
    }
    curClass->methods[curMethod->name] = curMethod;

    for (auto& var: *n->vars) {
        var->Accept(this);
    }

    for (auto& arg: *n->args) {
        curVar = new VariableInfo(dynamic_cast<Type*>(arg.first.get()), dynamic_cast<Identifier*>(arg.second.get())->name);
        if (curMethod->args.find(curVar->symbol) != curMethod->args.end()) {
            std::cout << "error: Arg was already declared.\n";
            errors.push_back("error: Arg was already declared.\n");
        }
        curMethod->args[curVar->symbol] = curVar;
    }

    curMethod = nullptr;
}


void TableBuilder::visit(const Type*) {assert(false);}

void TableBuilder::visit(const IfStatement*) {assert(false);}
void TableBuilder::visit(const WhileStatement*) {assert(false);}
void TableBuilder::visit(const Statement*) {assert(false);}
void TableBuilder::visit(const PrintStatement*) {assert(false);}
void TableBuilder::visit(const AssignmentStatement*) {assert(false);}
void TableBuilder::visit(const ArrAssignmentStatement*) {assert(false);}

void TableBuilder::visit(const AndExpression*) {assert(false);}
void TableBuilder::visit(const LessExpression*) {assert(false);}
void TableBuilder::visit(const PlusExpression*) {assert(false);}
void TableBuilder::visit(const MinusExpression*) {assert(false);}
void TableBuilder::visit(const MultExpression*) {assert(false);}
void TableBuilder::visit(const OrExpression*) {assert(false);}
void TableBuilder::visit(const RemainExpression*) {assert(false);}
void TableBuilder::visit(const ArrayExpression*) {assert(false);}
void TableBuilder::visit(const LengthExpression*) {assert(false);}
void TableBuilder::visit(const MethodExpression*) {assert(false);}
void TableBuilder::visit(const Integer*) {assert(false);}
void TableBuilder::visit(const Bool*) {assert(false);}
void TableBuilder::visit(const IdentExpression*) {assert(false);}
void TableBuilder::visit(const This*) {assert(false);}
void TableBuilder::visit(const NewArrExpression*) {assert(false);}
void TableBuilder::visit(const NewExpression*) {assert(false);}
void TableBuilder::visit(const NotExpression*) {assert(false);}
void TableBuilder::visit(const Expression*)  {assert(false);}

void TableBuilder::visit(const Identifier*)  {assert(false);}