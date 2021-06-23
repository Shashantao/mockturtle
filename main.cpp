#include <mockturtle/mockturtle.hpp>
#include <mockturtle/traits.hpp>
#include <lorina/aiger.hpp>

using namespace mockturtle;

void TestRead() {

  mig_network mig;
  std::basic_string<char> input_verilog;
  std::cout << "Input VERILOG file(.v) name: ";
  std::cin >> input_verilog;

  /* read a network from a file */
  lorina::read_verilog(input_verilog, verilog_reader(mig));

  const auto cuts = cut_enumeration(mig);
  mig.foreach_node([&](auto node) {
    std::cout << cuts.cuts(mig.node_to_index(node)) << "\n";
  });

}

void DepthRewrite(mig_network& mig, char strat = '0') {
  /* Create a depth view on the MIG so that it supports level() */
  depth_view mig_depth{mig};

  /* Rewrite the network into MIG */
  mig_algebraic_depth_rewriting_params ps;
  if (strat == '0')
  {
    std::cout << "Depth rewriting strategy: \n"
                 "(a) DFS\n"
                 "(b) Aggressive\n"
                 "(c) Selective\n";
    std::cin >> strat;
  }
  ps.allow_area_increase = true;
  if (strat == 'c') ps.strategy = ps.selective;
  else if (strat == 'b') ps.strategy = ps.aggressive;
  else ps.strategy = ps.dfs;
  mig_algebraic_depth_rewriting(mig_depth, ps);

  /*
  write_verilog(mig_depth, "output.v");
  std::cout << "Result is written into \"output.v\"\n";
  exit(0);
  */

  mig = mig_depth;
  mig = cleanup_dangling( mig );
}

void NodeResynth(mig_network& mig, int cut_size = 0) {
  /* LUT mapping */
  mapping_view<mig_network, true> mapped_mig{mig};
  lut_mapping_params ps;
  if (cut_size <= 0)
  {
    std::cout << "Cut enumeration size (<=4): ";
    std::cin >> cut_size;
  }
  ps.cut_enumeration_ps.cut_size = cut_size;
  lut_mapping<mapping_view<mig_network, true>, true>( mapped_mig, ps );

  /* collapse into k-LUT network */
  const auto klut = *collapse_mapped_network<klut_network>( mapped_mig );

  /* node resynthesis */
  mig_npn_resynthesis resyn;
  mig = node_resynthesis<mig_network>( klut, resyn );

  mig = cleanup_dangling( mig );

  // NOTE by HC:
  // At this point, mig is mostly resynthesized into MIG circuit,
  // with a few AND and OR logic unchanged.
  // We can optimize the circuit based on mig
}

void CutRewriting(mig_network& mig, int cut_size = 0){
  if (cut_size <= 0)
  {
    std::cout << "Cut enumeration size (<=4): ";
    std::cin >> cut_size;
  }
  /* node resynthesis */
  mig_npn_resynthesis resyn;
  cut_rewriting_params ps;
  ps.cut_enumeration_ps.cut_size = cut_size;
  mig = cut_rewriting( mig, resyn, ps );
  mig = cleanup_dangling( mig );
}

void Refactoring(mig_network& mig) {
  /* node resynthesis */
  mig_npn_resynthesis resyn;
  refactoring( mig, resyn );
  mig = cleanup_dangling( mig );
}

void Resubstitution(mig_network& mig) {
  depth_view mig_depth{mig};
  fanout_view mig_fanout{mig_depth};
  /* resubstitute nodes */
  mig_resubstitution( mig_fanout );
  mig = mig_fanout;
  mig = cleanup_dangling( mig );
}

void FunctionalReduction(mig_network& mig) {
  functional_reduction( mig );
}

void AutoMode(mig_network& mig, int effort) {
  for (int i = 0; i < effort; ++i) {
    NodeResynth(mig, 4);
    DepthRewrite(mig, 'b');
  }
  //FunctionalReduction(mig);
  Resubstitution(mig);
}

int main() {
  /* choose a network type */
  mig_network mig;

  std::basic_string<char> input_verilog;
  std::cout << "Input VERILOG file(.v) name: ";
  std::cin >> input_verilog;

  /* read a network from a file */
  auto feedback = lorina::read_verilog(input_verilog, verilog_reader(mig));

  if (feedback == lorina::return_code::parse_error) {
    std::cout << "Failed to read " << input_verilog << "\n";
    exit(0);
  } else {
    std::cout << "Successfully read " << input_verilog << "\n";
  }

  int option;
  std::string history;

  char mode;
  std::cout << "Automatically proccess? (Y/N): ";
  std::cin >> mode;

  if (mode == 'Y' || mode == 'y') {
    int effort;
    std::cout << "Input the effort (a positive integer): ";
    std::cin >> effort;
    AutoMode(mig, effort);
    write_verilog(mig, "output.v");
    std::cout << "Result is written into \"output.v\"\n";
    return 0;
  } else if (mode == 'N' || mode == 'n') {
    std::cout << "Manual mode\n";
  } else {
    std::cout << "Invalid input. Exiting program.\n";
    exit(0);
  }
  bool hasFR = false;
  do
  {
    std::cout << "==========================================\n";
    std::cout << "Operation history: " << history << "\n";
    std::cout << "==========================================\n";
    std::cout << "(1) Mig algebraic depth rewriting\n"
                 "(2) Mig node resynthesis\n"
                 "(3) Cut rewriting\n"
                 "(4) Refactoring\n"
                 "(5) Resubstitution (Cannot be run after functional reduction)\n"
                 "(6) Functional Reduction\n"
                 "(0) Exit\n";
    std::cout << "Select the next operation: ";
    std::cin >> option;
    switch ( option )
    {
    case 0:
      /* write into file */
      write_verilog(mig, "output.v");
      std::cout << "Result is written into \"output.v\"\n";
      break;
    case 1:
      DepthRewrite(mig);
      break;
    case 2:
      NodeResynth(mig);
      break;
    case 3:
      CutRewriting(mig);
      break;
    case 4:
      Refactoring(mig);
      break;
    case 5:
      if (hasFR) {
        std::cout << "Resubstitution cannot be run after functional reduction\n";
        break;
      }
      Resubstitution(mig);
      break;
    case 6:
      FunctionalReduction(mig);
      hasFR = true;
      break;
    default:
      break;
    }
    history += std::to_string(option);
  } while (option > 0);

  return 0;
}


