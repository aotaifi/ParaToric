// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis

#define BOOST_TEST_MODULE TestCliOptions

#include "paratoric/types/types.hpp"

#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <type_traits>

namespace po = boost::program_options;

BOOST_AUTO_TEST_CASE(n_thermalization_accepts_int64_cli_value) {
    static_assert(std::is_same_v<decltype(paratoric::SimSpec::N_thermalization), std::int64_t>);

    std::int64_t N_thermalization = 0;
    po::options_description options;
    options.add_options()("N_thermalization", po::value(&N_thermalization));

    constexpr const char* argv[] = {
        "paratoric", "--N_thermalization", "3000000000"
    };
    po::variables_map values;
    po::store(po::parse_command_line(3, argv, options), values);
    po::notify(values);

    const paratoric::SimSpec sim_spec{.N_thermalization = N_thermalization};
    BOOST_TEST(N_thermalization == 3'000'000'000LL);
    BOOST_TEST(sim_spec.N_thermalization == 3'000'000'000LL);
}
