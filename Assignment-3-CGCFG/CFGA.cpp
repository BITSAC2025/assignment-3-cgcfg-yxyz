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

    // TODO: complete the following method: 'CFGAnalysis::analyze'
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
        for (auto snk : sinks)
        {
            // TODO: DFS the graph, starting from src and detecting the paths ending at snk.
            // Use the class method 'recordPath' (already defined) to record the path you detected.
            //@{
            // Clear call stack for each src-snk pair
            while (!callStack.empty())
                callStack.pop();
            
            set<unsigned> visited;
            vector<unsigned> path;
            dfs(icfg, src, snk, visited, path);
            //@}
        }
}

void CFGAnalysis::dfs(SVF::ICFG *icfg, unsigned src, unsigned snk, 
                      set<unsigned> &visited, vector<unsigned> &path)
{
    // Mark current node as visited and add to path
    visited.insert(src);
    path.push_back(src);
    
    // If we reached the sink, record the path
    if (src == snk)
    {
        recordPath(path);
    }
    else
    {
        // Get the current node
        ICFGNode *node = icfg->getICFGNode(src);
        
        // Iterate through outgoing edges
        for (auto it = node->OutEdgeBegin(); it != node->OutEdgeEnd(); ++it)
        {
            ICFGEdge *edge = *it;
            unsigned dst = edge->getDstNode()->getId();
            
            // Check if it's a call edge
            if (auto callEdge = dyn_cast<CallCFGEdge>(edge))
            {
                // Push the call site ID onto the call stack
                callStack.push(callEdge->getCallSiteID());
                
                // Continue DFS if destination not visited
                if (visited.find(dst) == visited.end())
                {
                    dfs(icfg, dst, snk, visited, path);
                }
                
                // Pop the call site ID from the call stack
                callStack.pop();
            }
            // Check if it's a return edge
            else if (auto retEdge = dyn_cast<RetCFGEdge>(edge))
            {
                // Context-sensitive: only allow return if call site ID matches
                if (!callStack.empty() && callStack.top() == retEdge->getCallSiteID())
                {
                    // Pop the matching call site ID
                    callStack.pop();
                    
                    // Continue DFS if destination not visited
                    if (visited.find(dst) == visited.end())
                    {
                        dfs(icfg, dst, snk, visited, path);
                    }
                    
                    // Push back the call site ID for backtracking
                    callStack.push(retEdge->getCallSiteID());
                }
                // If call site ID doesn't match, skip this edge (context-sensitive filtering)
            }
            // For other edge types (intra-procedural edges)
            else
            {
                // Continue DFS if destination not visited
                if (visited.find(dst) == visited.end())
                {
                    dfs(icfg, dst, snk, visited, path);
                }
            }
        }
    }
    
    // Backtrack: remove from visited and path
    visited.erase(src);
    path.pop_back();
}
