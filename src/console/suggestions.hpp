#ifndef AF2_CONSOLE_SUGGESTIONS_HPP
#define AF2_CONSOLE_SUGGESTIONS_HPP

#include "../utilities/algorithm.hpp" // util::append
#include "../utilities/tuple.hpp"     // util::forEach, util::ctie
#include "script.hpp"                 // Script

#include <cstddef> // std::size_t
#include <string>  // std::string
#include <utility> // std::forward
#include <vector>  // std::vector

class ConCommand;
class Game;
class GameServer;
class GameClient;
class MetaServer;
class MetaClient;
class VirtualMachine;

#define SUGGESTIONS(name) \
	auto name([[maybe_unused]] const ConCommand& self, \
	          [[maybe_unused]] const Script::Command& command, \
	          [[maybe_unused]] std::size_t i, \
	          [[maybe_unused]] Game& game, \
	          [[maybe_unused]] GameServer* server, \
	          [[maybe_unused]] GameClient* client, \
	          [[maybe_unused]] MetaServer* metaServer, \
	          [[maybe_unused]] MetaClient* metaClient, \
	          [[maybe_unused]] VirtualMachine& vm) \
		->Suggestions
#define SUGGEST(...) (Suggestions::combine<__VA_ARGS__>())

class Suggestions final {
public:
	using value_type = std::string;

private:
	using Container = std::vector<value_type>;

public:
	using size_type = Container::size_type;
	using difference_type = Container::difference_type;
	using reference = Container::reference;
	using const_reference = Container::const_reference;
	using pointer = Container::pointer;
	using const_pointer = Container::const_pointer;
	using iterator = Container::iterator;
	using const_iterator = Container::const_iterator;
	using reverse_iterator = Container::reverse_iterator;
	using const_reverse_iterator = Container::const_reverse_iterator;

	using Function = Suggestions (*)(const ConCommand& self, const Script::Command& command, std::size_t i, Game& game, GameServer* server,
	                                 GameClient* client, MetaServer* metaServer, MetaClient* metaClient, VirtualMachine& vm);

	Suggestions() noexcept = default;

	explicit Suggestions(std::vector<value_type> suggestions)
		: m_suggestions(std::move(suggestions)) {}

	Suggestions(std::initializer_list<value_type> init)
		: m_suggestions(init) {}

	template <typename... Args>
	auto emplace_back(Args&&... args) -> decltype(auto) {
		return m_suggestions.emplace_back(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto push_back(Args&&... args) -> decltype(auto) {
		return m_suggestions.push_back(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto insert(Args&&... args) -> decltype(auto) {
		return m_suggestions.insert(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto erase(Args&&... args) -> decltype(auto) {
		return m_suggestions.erase(std::forward<Args>(args)...);
	}

	[[nodiscard]] auto front() -> decltype(auto) {
		return m_suggestions.front();
	}

	[[nodiscard]] auto front() const -> decltype(auto) {
		return m_suggestions.front();
	}

	[[nodiscard]] auto back() -> decltype(auto) {
		return m_suggestions.back();
	}

	[[nodiscard]] auto back() const -> decltype(auto) {
		return m_suggestions.back();
	}

	[[nodiscard]] auto begin() noexcept -> decltype(auto) {
		return m_suggestions.begin();
	}

	[[nodiscard]] auto begin() const noexcept -> decltype(auto) {
		return m_suggestions.begin();
	}

	[[nodiscard]] auto end() noexcept -> decltype(auto) {
		return m_suggestions.end();
	}

	[[nodiscard]] auto end() const noexcept -> decltype(auto) {
		return m_suggestions.end();
	}

	[[nodiscard]] auto size() const noexcept -> decltype(auto) {
		return m_suggestions.size();
	}

	[[nodiscard]] auto empty() const noexcept -> decltype(auto) {
		return m_suggestions.empty();
	}

	[[nodiscard]] auto clear() noexcept -> decltype(auto) {
		return m_suggestions.clear();
	}

	template <typename Index>
	[[nodiscard]] auto operator[](Index&& i) -> decltype(auto) {
		return m_suggestions[std::forward<Index>(i)];
	}

	template <typename Index>
	[[nodiscard]] auto operator[](Index&& i) const -> decltype(auto) {
		return m_suggestions[std::forward<Index>(i)];
	}

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestFile) {
		if (i == INDEX) {
			return Suggestions::getDataFiles();
		}
		return Suggestions{};
	}

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestScriptFile) {
		if (i == INDEX) {
			return Suggestions::getScriptFilenames();
		}
		return Suggestions{};
	}

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestMap) {
		if (i == INDEX) {
			return Suggestions::getMapFilenames();
		}
		return Suggestions{};
	}

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestSoundFile) {
		if (i == INDEX) {
			return Suggestions::getSoundFilenames();
		}
		return Suggestions{};
	}

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestShaderFile) {
		if (i == INDEX) {
			return Suggestions::getShaderFilenames();
		}
		return Suggestions{};
	}

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestCommand) {
		if (i == INDEX) {
			return Suggestions::getCommandNames();
		}
		return Suggestions{};
	}

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestCvar) {
		if (i == INDEX) {
			return Suggestions::getCvarNames();
		}
		return Suggestions{};
	}

	[[nodiscard]] static auto getFiles(std::string_view directory, std::string_view relativeTo) -> Suggestions;
	[[nodiscard]] static auto getDataFiles() -> Suggestions;
	[[nodiscard]] static auto getScriptFilenames() -> Suggestions;
	[[nodiscard]] static auto getMapFilenames() -> Suggestions;
	[[nodiscard]] static auto getSoundFilenames() -> Suggestions;
	[[nodiscard]] static auto getShaderFilenames() -> Suggestions;
	[[nodiscard]] static auto getCommandNames() -> Suggestions;
	[[nodiscard]] static auto getCvarNames() -> Suggestions;

	template <Function... FUNCS>
	[[nodiscard]] static constexpr auto combine() noexcept -> Function {
		struct Combination final {
			[[nodiscard]] static auto suggestCombined(const ConCommand& self, const Script::Command& command, std::size_t i, Game& game,
			                                          GameServer* server, GameClient* client, MetaServer* metaServer,
			                                          MetaClient* metaClient, VirtualMachine& vm) -> Suggestions {
				auto result = Suggestions{};
				util::forEach(util::ctie(FUNCS...), [&](Function func, std::size_t) {
					if (result.empty()) {
						result = func(self, command, i, game, server, client, metaServer, metaClient, vm);
					} else {
						util::append(result, func(self, command, i, game, server, client, metaServer, metaClient, vm));
					}
				});
				return result;
			}
		};
		return &Combination::suggestCombined;
	}

private:
	Container m_suggestions{};
};

#endif
