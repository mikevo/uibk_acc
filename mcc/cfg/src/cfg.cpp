#include "mcc/cfg/cfg.h"

#include <cassert>
#include <memory>
#include <ostream>
#include <typeinfo>
#include <vector>

#include "mcc/tac/operator.h"

namespace mcc {
  namespace cfg {
    Cfg::Cfg(mcc::tac::Tac tac) :
        basicBlockIndex(tac.getBasicBlockIndex()) {

      for (auto block : basicBlockIndex) {
        boost::add_vertex(block, graph);
      }

      unsigned prevBlockId = 0;
      bool prevMatched = false;

      for (auto line : tac.codeLines) {
        bool matched = false;

        auto op = line.get()->op;

        if (op.getName() == mcc::tac::OperatorName::JUMP) {
          if (typeid(*line.get()->arg1.get()) == typeid(mcc::tac::Label)) {
            auto label = std::static_pointer_cast<mcc::tac::Label>(
                line.get()->arg1);

            boost::add_edge(line.get()->basicBlockId, label.get()->basicBlockId,
                graph);
            matched = true;
          } else {
            assert(false && "Unknown jump destination");
          }
        }

        if (op.getName() == mcc::tac::OperatorName::JUMPFALSE) {
          if (typeid(*line.get()->arg2.get()) == typeid(mcc::tac::Label)) {
            auto label = std::static_pointer_cast<mcc::tac::Label>(
                line.get()->arg2);

            boost::add_edge(line.get()->basicBlockId,
                line.get()->basicBlockId + 1, graph);

            boost::add_edge(line.get()->basicBlockId, label.get()->basicBlockId,
                graph);
            matched = true;
          } else {
            assert(false && "Unknown jump destination");
          }
        }

        if (prevBlockId < line->basicBlockId) {
          if (!prevMatched) {
            boost::add_edge(prevBlockId, line.get()->basicBlockId, graph);
          }

          prevBlockId = line->basicBlockId;
        }

        prevMatched = matched;
      }
    }

    std::string Cfg::toDot() const {
      std::ostringstream out;
      boost::write_graphviz(out, graph);
      return out.str();
    }

    void Cfg::storeDot(std::string fileName) const {
      std::ofstream outf(fileName);

      outf << toDot();
    }

    void Cfg::calculateDOM() {

      boost::associative_property_map<VertexVertexMap> domTreePredMap(
          dominatorTree);

      boost::lengauer_tarjan_dominator_tree(graph, boost::vertex(0, graph),
          domTreePredMap);
    }

    VertexVertexMap& Cfg::getDomTree() {
      if (!dominatorTree.empty()) {
        return dominatorTree;
      }

      calculateDOM();

      return dominatorTree;
    }

    VertexDescriptor Cfg::getIdom(VertexDescriptor vertex) {
      if (dominatorTree.empty()) {
        calculateDOM();
      }

      return dominatorTree[vertex];
    }

    const Vertex& Cfg::getIdom(Vertex& vertex) {
      auto idom = getIdom(vertex->getBlockId());
      return basicBlockIndex[idom];
    }

    std::set<VertexDescriptor> Cfg::getDomSet(VertexDescriptor vertex) {
      std::set<VertexDescriptor> domSet;

      VertexDescriptor idom = getIdom(vertex);
      while (idom > getIdom(idom)) {
        domSet.insert(idom);
        idom = getIdom(idom);
      }

      domSet.insert(idom);

      return domSet;
    }

    std::set<Vertex> Cfg::getDomSet(Vertex vertex) {
      std::set<Vertex> domSet;
      auto dSet = getDomSet(vertex->getBlockId());

      for(auto e : dSet) {
        domSet.insert(basicBlockIndex[e]);
      }

      return domSet;
    }

  }
}
