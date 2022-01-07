#include "game/game.hpp" // Game
#include "logger.hpp"    // logger::...

#include <cstdlib>   // EXIT_FAILURE
#include <iostream>  // std::cerr, std::endl
#include <stdexcept> // std::exception

auto main(int argc, char* argv[]) -> int {
	try {
		auto game = Game{argc, argv};
		return game.run();
	} catch (const std::exception& e) {
		const auto* const message = e.what();
		logger::logFatalError(message);
		std::cerr << message << std::endl;
	} catch (...) {
		const auto* const message = "Unknown exception thrown!";
		logger::logFatalError(message);
		std::cerr << message << std::endl;
	}
	return EXIT_FAILURE;
}
