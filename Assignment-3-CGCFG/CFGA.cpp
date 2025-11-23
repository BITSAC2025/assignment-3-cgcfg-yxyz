/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"
#include <set>
#include <vector>

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    // TODO: 完成以下方法: 'CFGAnalysis::analyze'
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    // 源节点和汇节点在分析器实例化时指定。
    for (auto startNode : sources)
        for (auto targetNode : sinks)
        {
            // TODO: 对图进行深度优先搜索，从源节点开始，检测到汇节点结束的路径。
            // 使用类方法 'recordPath'（已定义）来记录检测到的路径。
            //@{
            std::set<unsigned> seenNodes;
            std::vector<unsigned> currentPath;
            dfs(startNode, targetNode, icfg, seenNodes, currentPath);
            //@}
        }
}

void CFGAnalysis::dfs(unsigned startNode, unsigned targetNode, SVF::ICFG *icfg,
                      std::set<unsigned> &seenNodes, std::vector<unsigned> &currentPath) {
    if (!seenNodes.insert(startNode).second) {
        return; 
    }

    currentPath.push_back(startNode);

    if (startNode == targetNode) {
        recordPath(currentPath);
    } else {
        const auto *curNode = icfg->getICFGNode(startNode);
        const auto &outgoingEdges = curNode->getOutEdges();

        for (const auto *edge : outgoingEdges) {
            const auto *nextNode = edge->getDstNode();
            const unsigned nodeId = nextNode->getId();

            if (edge->isIntraCFGEdge()) {
                dfs(nodeId, targetNode, icfg, seenNodes, currentPath);
            } 
            else if (edge->isCallCFGEdge()) {
                const auto *callEdge = llvm::dyn_cast<CallCFGEdge>(edge);
                const unsigned callSiteId = callEdge->getCallSite()->getId();

                callStack.push(callSiteId);
                dfs(nodeId, targetNode, icfg, seenNodes, currentPath);
                callStack.pop();
            } 
            else if (edge->isRetCFGEdge()) {
                const auto *retEdge = llvm::dyn_cast<RetCFGEdge>(edge);
                const unsigned callSiteId = retEdge->getCallSite()->getId();

                if (callStack.empty()) {
                    dfs(nodeId, targetNode, icfg, seenNodes, currentPath);
                } 
                else if (callStack.top() == callSiteId) {
                    callStack.pop();
                    dfs(nodeId, targetNode, icfg, seenNodes, currentPath);
                    callStack.push(callSiteId);
                }
            }
        }
    }

    seenNodes.erase(startNode);
    currentPath.pop_back();
}
