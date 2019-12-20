#include "clang/Driver/Options.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include <fstream>


using namespace llvm;
using namespace clang;
using namespace clang::tooling;

//重写的静态对象
static clang::Rewriter TheRewriter;

//filePath
static std::string filePath;

//OptionCategory指定了工具所处的分类随意就好
static cl::OptionCategory OptsCategory("ClangAutoStats_qzd");



namespace ClangPluginQZD{

//断点调试代码
void breakPoint (std::string str,bool bool1){
    errs() << str << "\n";
    if (!bool1) {
        return;
    }
    int i = 8;
    assert(i < 0 && "有效");
}

#pragma mark----------------RecursiveASTVisitor
/*
 RecursiveASTVisitor 是一个深度优先遍历 AST 和访问节点的类
 1遍历（Traverse） AST 的每个节点；
 2回溯（WalkUp） 在某一个节点，访问这个节点的层次结构 ( 每个节点也是一个树 )；
 3访问（Visit）如果某一个节点是一种类型的动态类别 ( 比如是一个子类等 )，调用一个用户重载的函数来访问这个节点；
 */
class ClangAutoStatsVisitor
: public RecursiveASTVisitor<ClangAutoStatsVisitor> {
private:
ASTContext *astContext; // used for getting additional AST info
//typedef clang::RecursiveASTVisitor<RewritingVisitor> Base;
Rewriter &rewriter;
CompilerInstance *CI;
public:
    explicit ClangAutoStatsVisitor(Rewriter &R)
       : rewriter{R} // initialize private members
       {}
    explicit ClangAutoStatsVisitor(ASTContext *Ctx,Rewriter &R,CompilerInstance *aCI) :
    astContext(Ctx),rewriter{R},CI(aCI){
        //breakPoint("AST init 生成", true);
    }
    //每个文件在parse完之后，做一些清理和内存释放工作 -obj ParseSyntaxOnly
    void EndSourceFileAction()  {
         //breakPoint("EndSourceFileAction 执行", true);
        
        SourceManager &SM = TheRewriter.getSourceMgr();
        std::string ClasFilePath;
        size_t pos = filePath.find_last_of(".");

        llvm::errs() << "** EndSourceFileAction for: "
        << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

        std::string Filename = SM.getFileEntryForID(SM.getMainFileID())->getName();

        pos = Filename.find_last_of(".");

        if (pos != std::string::npos) {
                  ClasFilePath = Filename.substr(0,pos) +".clas";
        }

        std::error_code error_code;
        llvm::raw_fd_ostream outFile(Filename, error_code, llvm::sys::fs::F_None);
        // 将Rewriter结果输出到文件中
        TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);

    }
    bool VisitObjCImplementationDecl(ObjCImplementationDecl *ID) {
         errs() << "处理 visit decl" << "\n";
        for (auto D : ID->decls()) {
            if (ObjCMethodDecl *MD = dyn_cast<ObjCMethodDecl>(D)) {
                handleObjcMethDecl(MD);
            }
        }
        //breakPoint("VisitObjCImplementationDecl 完成", true);
        EndSourceFileAction();
        return true;
    }
    bool handleObjcMethDecl(ObjCMethodDecl *MD) {
        if (!MD->hasBody()) return true;
        errs() << MD->getNameAsString() << "\n";
        //复合语句stmt
        CompoundStmt *cmpdStmt = MD->getCompoundBody();
        //指向大括号后的位置
        SourceLocation loc = cmpdStmt->getBeginLoc().getLocWithOffset(1);
     
        //如果是宏定义展开的话
        if (loc.isMacroID()) {
           // loc = rewriter.getSourceMgr().getImmediateExpansionRange(loc).first;
            loc = rewriter.getSourceMgr().getImmediateExpansionRange(loc).getBegin();
            errs() << "is MarcroId 宏定义" << ":++++\n";
        }
       //static std::string varName("%__FUNCNAME__%");
        //
        static std::string varName("printf(\"文件名:%s, 方法名:%s, 行数:%d, 时间:%s,\\n\", [NSString stringWithUTF8String:__FILE__].lastPathComponent.UTF8String,__PRETTY_FUNCTION__,__LINE__,__TIME__);");
       std::string funcName = MD->getDeclName().getAsString();
       
       Stmt *methodBody = MD->getBody();
        
       std::string srcCode;
  
        
       //获取源码片段
   srcCode.assign(astContext->getSourceManager().getCharacterData(methodBody->getSourceRange().getBegin()),methodBody->getSourceRange().getEnd().getRawEncoding()-methodBody->getSourceRange().getBegin().getRawEncoding()+1);
        
        
       
       std::string codes(srcCode);
       size_t pos = 0;
       bool isTaged = false;
       while ((pos = codes.find(varName, pos)) != std::string::npos) {
           codes.replace(pos, varName.length(), funcName);
           pos += funcName.length();
           isTaged = true;
       }
       errs() << "+++:"+srcCode << ":++++\n";
       
        if (isTaged) {
            
            return true;
        }
      
        if (rewriter.isRewritable(loc)) {
             errs() <<  "可以写入" << ":++++\n";
        }else{
            errs() <<  "不可以写入" << ":++++\n";
        }
        errs() <<  "test" << ":++++\n";
        const char * char1 = TheRewriter.getSourceMgr().getCharacterData(loc);
        std::string char2(char1);
        errs() << ":--"+char2 +"--:"  << "\n";
        TheRewriter.InsertTextBefore(loc, varName);//代码插入
        errs() << "结束插入" << ":++++\n";

        return true;
    }
    
    
};
#pragma mark----------------ASTConsumer
class ClangAutoStatsASTConsumer : public ASTConsumer {
private:
    ClangAutoStatsVisitor Visitor;
    CompilerInstance *CI;
public:
    // override the constructor in order to pass CI
       explicit ClangAutoStatsASTConsumer(Rewriter &R)
       : Visitor(R) // initialize the visitor
       {}
    explicit ClangAutoStatsASTConsumer(CompilerInstance *aCI,Rewriter &R)
    : Visitor(&(aCI->getASTContext()),R,aCI), CI(aCI) {}
    //这是 ClangAutoStatsASTConsumer 的入口。当整个抽象语法树 (AST) 构造完成以后，HandleTranslationUnit 这个函数将会被 Clang 的驱动程序调用 replace未调用
    void HandleTranslationUnit(ASTContext &context) override {
      TranslationUnitDecl *decl = context.getTranslationUnitDecl();
      Visitor.TraverseTranslationUnitDecl(decl);
       // breakPoint("HandleTranslationUnit 执行", true);
        errs() << "HandleTranslationUnit...." << "\n";
    }
    
  
    
   /**
    HandleTopLevelDecl是在遍历到Decl（即声明或定义，例如函数、ObjC interface等）的时候立即回调，而HandleTranslationUnit则是在当前的TranslationUnit（即目标文件或源代码）的AST被完整解析出来后才会回调。
    TopLevel指的是在AST第一层的节点，对于OC代码来说，这一般是interface、implementation、全局变量等在代码最外层的声明或定义
    */
    
    
};


#pragma mark----------------PluginASTAction
//ASTFrontendAction是用来为前端工具定义标准化的AST操作流程的。一个前端可以注册多个Action，然后在指定时刻轮询调用每一个Action的特定方法
class ClangAutoStatsAction : public PluginASTAction{
private:
    Rewriter rewriter;
public:
    //返回自己创建并返回给前端一个ASTConsumer
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        
        rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        CI.getPreprocessor();
        errs() << "create CreateASTConsumer .... success" << "\n";
        std::string fileName(file.data());
        errs() << "("+fileName+")" << "\n";
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return llvm::make_unique<ClangAutoStatsASTConsumer>(&CI,rewriter);
    }
   bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args) override{

       return true;
    }
    
    bool PrepareToExecuteAction(CompilerInstance &CI) override{
        //Act->PrepareToExecute(CI);
        return true;
        
    }
    
     bool BeginInvocation(CompilerInstance &CI) override {
         //breakPoint("BeginInvocation 执行", true);
         //Act->BeginInvocation(CI);
         return true;
    }
    PluginASTAction::ActionType getActionType() override {
        //breakPoint("getActionType 执行", true);AddAfterMainAction ReplaceAction AddBeforeMainAction
       return AddAfterMainAction;
     }
    
//    bool BeginSourceFile(CompilerInstance &CI, const FrontendInputFile &Input) {
//        breakPoint("BeginSourceFile 执行", true);
//        return true;
//    }
//     bool BeginSourceFileAction(CompilerInstance &CI) override {
//         breakPoint("BeginSourceFileAction 执行", true);
//         return true;
//    }
//
//    void ExecuteAction() override{
//         errs() << "action 处理 .... success" << "\n";
//    }
    
    
    
  //每个文件在parse完之后，做一些清理和内存释放工作 -obj ParseSyntaxOnly
  void EndSourceFileAction() override {
       //breakPoint("EndSourceFileAction 执行", true);
      
      SourceManager &SM = TheRewriter.getSourceMgr();
      std::string ClasFilePath;
      size_t pos = filePath.find_last_of(".")
      
      ;

      llvm::errs() << "** EndSourceFileAction for: "
      << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

      std::string Filename = SM.getFileEntryForID(SM.getMainFileID())->getName();

      pos = Filename.find_last_of(".");

      if (pos != std::string::npos) {
                ClasFilePath = Filename.substr(0,pos) +".clas";
      }

      std::error_code error_code;
      llvm::raw_fd_ostream outFile(ClasFilePath, error_code, llvm::sys::fs::F_None);
      // 将Rewriter结果输出到文件中
      TheRewriter.getEditBuffer(SM.getMainFileID()).write(outFile);
      
    }
};

}
static clang::FrontendPluginRegistry::Add
<ClangPluginQZD::ClangAutoStatsAction>Z("ClangAutoStats_qzd","ClangAutoStats_qzd plugin for auto log");


#warning TODO 1.目前只支持全量打点，定制化未实现，可以在工程内[LogUtil log:]配置
#warning TODO 2.编译工程时生成的打点代码第一次只是写入文件，未生成相应代码到编译器后端，需要手动再编译工程
#warning TODO 3.编译出的文件包含与源文件不符的Debug信息未测试


/*
 CLAS执行完成后，还有一个非常重要的任务，就是将原文件.m重命名后，将CLAS输出的临时文件重命名为原文件，拼接剩余参数并调用苹果原生的Clang（/usr/bin/clang），clang执行完成后，无论成功与否，将临时文件删除并将原文件.m复原，编译流程至此结束。
 
 1、集成Xcode编译链中
 2、编译出的文件包含与源文件不符的Debug信息
 后续参考pass BugpointPasses
 */


/*
 参考文档：https://www.jianshu.com/p/01c988cae897
 参考文档：https://www.jianshu.com/p/c16391437f6f
 */

/*自动打点方案C.L.A.S.缺点
 无法适用于条件打点---条件打点为动态打点，本方案为静态打点适合80%场景
 插入的代码可能会造成编译失败---引入了其他文件这个可以通过配置CLAS插入用户指定的#include或#import
 插入范围过大---优化设计
 编译出的文件包含与源文件不符的Debug信息---生成的DebugSymbols是与临时文件(.clas.m)的信息相符的，与源文件并不相符，这个就需要我们在生成dSYMs的时候，把所有的临时文件信息替换为原始文件信息，为了达到这个目的，我们需要修改LLVM的dsymutil替换系统原生的dsymutil
 插入代码导致二进制体积变大---一到两条语句为佳，避免在插入代码里直接构造含有复杂逻辑和功能的语句。{ [MCStatistik logEvent:@"%__FUNC_NAME__%"]; }
 */

/*打点方案设计
 适应项目内80%的打点需求
 对现有代码逻辑无侵入
 对现有编译工具链无侵入
 */
