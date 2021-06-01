#include <mockturtle/mockturtle.hpp>
#include <lorina/verilog.hpp>

int main() {

    std::cout << "Input Verilog filename: ";
    std::string verilog_read_filename;
    std::getline(std::cin, verilog_read_filename);
    mockturtle::mig_network mig;
    lorina::diagnostic_engine diag;
    lorina::return_code result = lorina::read_verilog(
            verilog_read_filename, mockturtle::verilog_reader( mig ), &diag
            );

    if (result == lorina::return_code::parse_error) {
        std::cerr << "Error parsing Verilog file " << verilog_read_filename
            << ". Abort.\n";
        return 1;
    }

    const auto cuts = cut_enumeration( mig );
    mig.foreach_node( [&]( auto node ) {
        std::cout << cuts.cuts( mig.node_to_index( node ) ) << "\n";
    } );

	return 0;
}
