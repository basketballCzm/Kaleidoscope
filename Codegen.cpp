#include "Codegen.h"
#include "AST.h"
#include "Error.h"
/// In the LLVM IR, numeric constants are represented with the ConstantFP class, Note that in the LLVM IR that constants are all uniqued together and shared. For this reason, the API uses the “foo::get(…)” idiom instead of “new foo(..)” or “foo::Create(..)”.
Value *NumberExprAST::codegen() {
    return ConstantFP::get(TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
    // look this variable up in the function
    Value *V = NamedValues[Name];
    if (!V) {
        LogErrorV("Unkonw variable name");
    }

    return V;
}

Value *BinaryExprAST::codegen() {
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();

    if (!L || !R) {
        return nullptr;
    }

    switch (Op) {
        case '+':
            return Builder.CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder.CreateFSub(L, R, "subtmp");
        case '*':
            return Builder.CreateFMul(L, R, "multmp");
        case '<':
            L = Builder.CreateFCmpULT(L, R, "cmptmp");
            // Covert bool 0/1 to dlouble 0.0 or 1.0
            return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

Value *CallExprAST::codegen() {
    //look up the name in the global module table
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF) {
        return LogErrorV("UnKonwn function referenced");
    }

    // If arguement mismatch error
    if (CalleeF->arg_size() != Args.size()) {
        return LogErrorV("Incorrect # arguments passed");
    }

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; i++) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back()) {
            return nullptr;
        }
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen() {
    //Make the function type double (double, double) etc
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(TheContext));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);
    Function *F      = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
    // set names for all arguments
    unsigned Idx = 0;
    for (auto &Arg : F->args()) {
        Arg.setName(Args[Idx++]);
    }

    return F;
}

Function *FunctionAST::codegen() {
    //First, check for an existing function from a previous 'extern' declaration
    Function *TheFunction = TheModule->getFunction(Proto->getName());
    if (!TheFunction) {
        TheFunction = Proto->codegen();
    }

    if (!TheFunction) {
        return nullptr;
    }

    if (!TheFunction->empty()) {
        return (Function *)LogErrorV("Function cannot be redefined.");
    }

    //Create a new basic block to start insertion into
    BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
    //The second line then tells the builder that new instructions should be inserted into the end of the new basic block.
    Builder.SetInsertPoint(BB);

    //Record the funnction arguments in the NameValues map
    NamedValues.clear();
    //we add the function arguments to the NamedValues map (after first clearing it out) so that they’re accessible to VariableExprAST nodes.
    for (auto &Arg : TheFunction->args()) {
        NamedValues[Arg.getName()] = &Arg;
    }

    if (Value *RetVal = Body->codegen()) {
        //Finish off the function
        Builder.CreateRet(RetVal);

        //Validate the generated code, checking for consistency
        verifyFunction(*TheFunction);
        return TheFunction;
    }
    //error reading body, remove function
    TheFunction->eraseFromParent();
    return nullptr;
}