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
    for (auto sourceNode : sources)
        for (auto sinkNode : sinks)
        {
            // TODO: 对图进行深度优先搜索，从源节点开始，检测到汇节点结束的路径。
            // 使用类方法 'recordPath'（已定义）来记录检测到的路径。
            //@{
            std::set<unsigned> seenNodes;
            std::vector<unsigned> currentPath;
            dfs(sourceNode, sinkNode, icfg, seenNodes, currentPath);
            //@}
        }
}

void CFGAnalysis::dfs(unsigned currentNode, unsigned targetNode, SVF::ICFG *icfg,
                      std::set<unsigned> &seenNodes, std::vector<unsigned> &currentPath) {
    // 如果节点在当前路径中已访问，则提前返回
    if (!seenNodes.insert(currentNode).second) {
        return; 
    }

    currentPath.push_back(currentNode);

    // 找到目标节点，记录路径
    if (currentNode == targetNode) {
        recordPath(currentPath);
        seenNodes.erase(currentNode);
        currentPath.pop_back();
        return;
    }

    // 探索出边
    const auto *curNode = icfg->getICFGNode(currentNode);
    const auto &outgoingEdges = curNode->getOutEdges();

    for (const auto *edge : outgoingEdges) {
        const auto *nextNode = edge->getDstNode();
        const unsigned nextNodeId = nextNode->getId();

        // 处理过程内边
        if (edge->isIntraCFGEdge()) {
            dfs(nextNodeId, targetNode, icfg, seenNodes, currentPath);
            continue;
        }

        // 处理调用边
        if (edge->isCallCFGEdge()) {
            const auto *callEdge = llvm::dyn_cast<CallCFGEdge>(edge);
            const unsigned callSiteId = callEdge->getCallSite()->getId();
            callStack.push(callSiteId);
            dfs(nextNodeId, targetNode, icfg, seenNodes, currentPath);
            callStack.pop();
            continue;
        }

        // 处理返回边
        if (edge->isRetCFGEdge()) {
            const auto *retEdge = llvm::dyn_cast<RetCFGEdge>(edge);
            const unsigned callSiteId = retEdge->getCallSite()->getId();
            
            // 如果调用栈为空或匹配调用点，则允许返回
            if (callStack.empty()) {
                dfs(nextNodeId, targetNode, icfg, seenNodes, currentPath);
            } else if (callStack.top() == callSiteId) {
                callStack.pop();
                dfs(nextNodeId, targetNode, icfg, seenNodes, currentPath);
                callStack.push(callSiteId);
            }
        }
    }

    // 回溯
    seenNodes.erase(currentNode);
    currentPath.pop_back();
}
