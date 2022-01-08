#ifndef AF2_CONSOLE_CONVAR_HPP
#define AF2_CONSOLE_CONVAR_HPP

#include "../game/data/color.hpp" // Color
#include "../network/crypto.hpp"  // crypto::...
#include "command.hpp"            // cmd::...

#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint16_t
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move, std::forward
#include <vector>        // std::vector

class Game;
class GameServer;
class GameClient;
class MetaServer;
class MetaClient;

class ConVar {
public:
	[[nodiscard]] static auto all() -> std::unordered_map<std::string_view, ConVar&>&;
	[[nodiscard]] static auto find(std::string_view name) -> ConVar*;

	enum class Type {
		STRING,
		BOOL,
		INT,
		FLOAT,
		CHAR,
		COLOR,
		HASH,
	};

	using Flags = std::uint16_t;
	enum Flag : Flags {
		NO_FLAGS = 0,
		ALL_FLAGS = static_cast<Flags>(~0),
		CHEAT = 1 << 0,                   // Changing the value is considered a cheat.
		READ_ONLY = 1 << 1,               // Read only, cannot be set by the user at all.
		INIT = 1 << 2,                    // Can only be changed during startup.
		ARCHIVE = 1 << 3,                 // Value is saved to a config file on shutdown.
		REPLICATED = 1 << 4,              // Value is networked to all clients and can only be changed by the server.
		NOT_RUNNING_GAME_SERVER = 1 << 5, // Cannot be changed while the game server is running.
		NOT_RUNNING_GAME_CLIENT = 1 << 6, // Cannot be changed while the game client is running.
		NOT_RUNNING_META_SERVER = 1 << 7, // Cannot be changed while the meta server is running.
		NOT_RUNNING_META_CLIENT = 1 << 8, // Cannot be changed while the meta client is running.
		READ_ADMIN_ONLY = 1 << 9,         // Only admins may read the value.
		WRITE_ADMIN_ONLY = 1 << 10,       // Only admins may change the value.
		NO_RCON_READ = 1 << 11,           // Cannot be read remotely.
		NO_RCON_WRITE = 1 << 12,          // Cannot be changed remotely.
		HASHED = 1 << 13,                 // Value is hashed upon being set and is shown as ***HASHED***. getRaw() returns an empty string.
		SECRET = 1 << 14,                 // Value is shown as ***SECRET*** unless getRaw() is used.

		NOT_RUNNING_GAME = NOT_RUNNING_GAME_SERVER | NOT_RUNNING_GAME_CLIENT,
		ADMIN_ONLY = READ_ADMIN_ONLY | WRITE_ADMIN_ONLY,
		NO_RCON = NO_RCON_READ | NO_RCON_WRITE,

		CLIENT_SETTING = ARCHIVE | WRITE_ADMIN_ONLY | NO_RCON,
		CLIENT_VARIABLE = WRITE_ADMIN_ONLY | NO_RCON,
		CLIENT_PASSWORD = SECRET | ADMIN_ONLY | NO_RCON,
		SERVER_SETTING = ARCHIVE | WRITE_ADMIN_ONLY,
		SERVER_VARIABLE = NO_FLAGS,
		SERVER_PASSWORD = SECRET | HASHED | ADMIN_ONLY | NO_RCON,
		HOST_SETTING = ARCHIVE | WRITE_ADMIN_ONLY | NO_RCON,
		HOST_VARIABLE = WRITE_ADMIN_ONLY | NO_RCON,
		SHARED_VARIABLE = REPLICATED,
	};
	using Callback = cmd::Result (*)(ConVar& self, std::string_view oldVal, Game& game, GameServer* server, GameClient* client,
	                                 MetaServer* metaServer, MetaClient* metaClient);

	ConVar(const ConVar&) = delete;
	ConVar(ConVar&&) = delete;

	auto operator=(const ConVar&) -> ConVar& = delete;
	auto operator=(ConVar&&) -> ConVar& = delete;

protected:
	ConVar(Type type, std::string name, std::string defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

public:
	virtual ~ConVar();

	auto set(std::string_view value, Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) -> cmd::Result;
	auto setSilent(std::string_view value) -> cmd::Result;

	auto overrideLocalValue(std::string_view value, Game& game, GameServer* server, GameClient* client, MetaServer* metaServer,
	                        MetaClient* metaClient) -> cmd::Result;
	auto overrideLocalValueSilent(std::string_view value) -> cmd::Result;
	auto restoreLocalValue(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) -> cmd::Result;
	auto restoreLocalValueSilent() -> cmd::Result;

	[[nodiscard]] auto getRawLocalValue() const noexcept -> std::string_view;

	[[nodiscard]] auto getString() const -> std::string;
	[[nodiscard]] auto formatFlags() const -> std::string;
	[[nodiscard]] auto format(bool admin, bool rcon, bool defaultValue, bool limits, bool flags, bool description) const -> std::string;

	[[nodiscard]] auto getType() const noexcept -> Type;
	[[nodiscard]] auto getName() const noexcept -> std::string_view;
	[[nodiscard]] auto getRaw() const noexcept -> std::string_view;
	[[nodiscard]] auto getFlags() const noexcept -> Flags;
	[[nodiscard]] auto getDescription() const noexcept -> std::string_view;
	[[nodiscard]] auto getDefaultValue() const noexcept -> std::string_view;

	[[nodiscard]] virtual auto getMinValue() const noexcept -> float;
	[[nodiscard]] virtual auto getMaxValue() const noexcept -> float;
	[[nodiscard]] virtual auto getBool() const noexcept -> bool;
	[[nodiscard]] virtual auto getInt() const noexcept -> int;
	[[nodiscard]] virtual auto getFloat() const noexcept -> float;
	[[nodiscard]] virtual auto getChar() const noexcept -> char;

protected:
	auto setString(const std::string& str) -> void;
	auto setString(std::string&& str) noexcept -> void;

private:
	virtual auto updateValue(std::string_view value) -> cmd::Result = 0;
	auto setValue(std::string_view value, Game* game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient)
		-> cmd::Result;

	Type m_type;        // What type of value is stored.
	std::string m_name; // Name used to identify the cvar. Should never contain whitespace.
	std::string m_string; // The main value is always stored as a readable string (unless hashed) for quick access when used in scripts, when printing, etc.
	Flags m_flags;                           // Switches that determine how the cvar is used and how it can be accessed.
	std::string m_description;               // Contains a human-readable explanation of what the cvar is used for.
	std::string m_defaultValue;              // Default value that the cvar is set to when initialized or reset.
	std::optional<std::string> m_localValue; // Used to store the old clientside value of replicated cvars while they are overridden by the server.
	Callback m_callback;                     // Function that is called when the cvar is modified. May be null.
};

class ConVarString final : private ConVar {
public:
	ConVarString(std::string name, std::string defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator std::string_view() const noexcept;

	[[nodiscard]] auto begin() const noexcept -> std::string_view::const_iterator;
	[[nodiscard]] auto end() const noexcept -> std::string_view::const_iterator;
	[[nodiscard]] auto cbegin() const noexcept -> std::string_view::const_iterator;
	[[nodiscard]] auto cend() const noexcept -> std::string_view::const_iterator;
	[[nodiscard]] auto rbegin() const noexcept -> std::string_view::const_reverse_iterator;
	[[nodiscard]] auto rend() const noexcept -> std::string_view::const_reverse_iterator;
	[[nodiscard]] auto crbegin() const noexcept -> std::string_view::const_reverse_iterator;
	[[nodiscard]] auto crend() const noexcept -> std::string_view::const_reverse_iterator;

	[[nodiscard]] auto operator[](std::size_t i) const -> const char&;
	[[nodiscard]] auto at(std::size_t i) const -> const char&;
	[[nodiscard]] auto front() const noexcept -> const char&;
	[[nodiscard]] auto back() const noexcept -> const char&;

	[[nodiscard]] auto data() const noexcept -> const char*;
	[[nodiscard]] auto size() const noexcept -> std::size_t;
	[[nodiscard]] auto length() const noexcept -> std::size_t;
	[[nodiscard]] auto max_size() const noexcept -> std::size_t;
	[[nodiscard]] auto empty() const noexcept -> bool;

	auto copy(char* destination, std::size_t count, std::size_t pos = 0) const -> std::size_t;

	[[nodiscard]] auto substr(std::size_t pos, std::size_t count = std::string_view::npos) const -> std::string_view;

	[[nodiscard]] auto compare(std::string_view str) const noexcept -> int;
	[[nodiscard]] auto compare(std::size_t pos, std::size_t count, std::string_view str) const noexcept -> int;
	[[nodiscard]] auto compare(std::size_t pos1, std::size_t count1, std::string_view str, std::size_t pos2, std::size_t count2) const noexcept -> int;
	[[nodiscard]] auto compare(const char* str) const noexcept -> int;
	[[nodiscard]] auto compare(std::size_t pos, std::size_t count, const char* str) const noexcept -> int;
	[[nodiscard]] auto compare(std::size_t pos1, std::size_t count1, const char* str, std::size_t pos2, std::size_t count2) const noexcept -> int;

	[[nodiscard]] auto find(std::string_view str, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find(char ch, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find(const char* str, std::size_t pos, std::size_t count) const -> std::size_t;
	[[nodiscard]] auto find(const char* str, std::size_t pos = 0) const -> std::size_t;

	[[nodiscard]] auto rfind(std::string_view str, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto rfind(char ch, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto rfind(const char* str, std::size_t pos, std::size_t count) const -> std::size_t;
	[[nodiscard]] auto rfind(const char* str, std::size_t pos = 0) const -> std::size_t;

	[[nodiscard]] auto find_first_of(std::string_view str, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_first_of(char ch, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_first_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t;
	[[nodiscard]] auto find_first_of(const char* str, std::size_t pos = 0) const -> std::size_t;

	[[nodiscard]] auto find_last_of(std::string_view str, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_last_of(char ch, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_last_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t;
	[[nodiscard]] auto find_last_of(const char* str, std::size_t pos = 0) const -> std::size_t;

	[[nodiscard]] auto find_first_not_of(std::string_view str, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_first_not_of(char ch, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_first_not_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t;
	[[nodiscard]] auto find_first_not_of(const char* str, std::size_t pos = 0) const -> std::size_t;

	[[nodiscard]] auto find_last_not_of(std::string_view str, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_last_not_of(char ch, std::size_t pos = 0) const -> std::size_t;
	[[nodiscard]] auto find_last_not_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t;
	[[nodiscard]] auto find_last_not_of(const char* str, std::size_t pos = 0) const -> std::size_t;

	friend auto operator==(const ConVarString& lhs, std::string_view rhs) noexcept -> bool;
	friend auto operator!=(const ConVarString& lhs, std::string_view rhs) noexcept -> bool;
	friend auto operator<(const ConVarString& lhs, std::string_view rhs) noexcept -> bool;
	friend auto operator<=(const ConVarString& lhs, std::string_view rhs) noexcept -> bool;
	friend auto operator>(const ConVarString& lhs, std::string_view rhs) noexcept -> bool;
	friend auto operator>=(const ConVarString& lhs, std::string_view rhs) noexcept -> bool;
	friend auto operator==(std::string_view lhs, const ConVarString& rhs) noexcept -> bool;
	friend auto operator!=(std::string_view lhs, const ConVarString& rhs) noexcept -> bool;
	friend auto operator<(std::string_view lhs, const ConVarString& rhs) noexcept -> bool;
	friend auto operator<=(std::string_view lhs, const ConVarString& rhs) noexcept -> bool;
	friend auto operator>(std::string_view lhs, const ConVarString& rhs) noexcept -> bool;
	friend auto operator>=(std::string_view lhs, const ConVarString& rhs) noexcept -> bool;

private:
	[[nodiscard]] auto getBool() const noexcept -> bool override;
	[[nodiscard]] auto getInt() const noexcept -> int override;
	[[nodiscard]] auto getFloat() const noexcept -> float override;
	[[nodiscard]] auto getChar() const noexcept -> char override;

	auto updateValue(std::string_view value) -> cmd::Result override;
};

class ConVarBool final : private ConVar {
public:
	ConVarBool(std::string name, bool defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator const bool&() const noexcept;

private:
	[[nodiscard]] auto getMinValue() const noexcept -> float override;
	[[nodiscard]] auto getMaxValue() const noexcept -> float override;

	[[nodiscard]] auto getBool() const noexcept -> bool override;
	[[nodiscard]] auto getInt() const noexcept -> int override;
	[[nodiscard]] auto getFloat() const noexcept -> float override;
	[[nodiscard]] auto getChar() const noexcept -> char override;

	auto updateValue(std::string_view value) -> cmd::Result override;

	bool m_value = false;
};

class ConVarInt final : private ConVar {
public:
	ConVarInt(std::string name, int defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator const int&() const noexcept;

private:
	[[nodiscard]] auto getBool() const noexcept -> bool override;
	[[nodiscard]] auto getInt() const noexcept -> int override;
	[[nodiscard]] auto getFloat() const noexcept -> float override;
	[[nodiscard]] auto getChar() const noexcept -> char override;

	auto updateValue(std::string_view value) -> cmd::Result override;

	int m_value = 0;
};

class ConVarIntMinMax final : private ConVar {
public:
	ConVarIntMinMax(std::string name, int defaultValue, Flags flags, std::string description, int minValue, int maxValue, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator const int&() const noexcept;

private:
	[[nodiscard]] auto getMinValue() const noexcept -> float override;
	[[nodiscard]] auto getMaxValue() const noexcept -> float override;

	[[nodiscard]] auto getBool() const noexcept -> bool override;
	[[nodiscard]] auto getInt() const noexcept -> int override;
	[[nodiscard]] auto getFloat() const noexcept -> float override;
	[[nodiscard]] auto getChar() const noexcept -> char override;

	auto updateValue(std::string_view value) -> cmd::Result override;

	int m_value = 0;
	int m_minValue = 0;
	int m_maxValue = 0;
};

class ConVarFloat final : private ConVar {
public:
	ConVarFloat(std::string name, float defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator const float&() const noexcept;

private:
	[[nodiscard]] auto getBool() const noexcept -> bool override;
	[[nodiscard]] auto getInt() const noexcept -> int override;
	[[nodiscard]] auto getFloat() const noexcept -> float override;
	[[nodiscard]] auto getChar() const noexcept -> char override;

	auto updateValue(std::string_view value) -> cmd::Result override;

	float m_value = 0.0f;
};

class ConVarFloatMinMax final : private ConVar {
public:
	ConVarFloatMinMax(std::string name, float defaultValue, Flags flags, std::string description, float minValue, float maxValue,
	                  Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator const float&() const noexcept;

private:
	[[nodiscard]] auto getMinValue() const noexcept -> float override;
	[[nodiscard]] auto getMaxValue() const noexcept -> float override;

	[[nodiscard]] auto getBool() const noexcept -> bool override;
	[[nodiscard]] auto getInt() const noexcept -> int override;
	[[nodiscard]] auto getFloat() const noexcept -> float override;
	[[nodiscard]] auto getChar() const noexcept -> char override;

	auto updateValue(std::string_view value) -> cmd::Result override;

	float m_value = 0.0f;
	float m_minValue = 0.0f;
	float m_maxValue = 0.0f;
};

class ConVarChar final : private ConVar {
public:
	ConVarChar(std::string name, char defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator const char&() const noexcept;

private:
	[[nodiscard]] auto getBool() const noexcept -> bool override;
	[[nodiscard]] auto getInt() const noexcept -> int override;
	[[nodiscard]] auto getFloat() const noexcept -> float override;
	[[nodiscard]] auto getChar() const noexcept -> char override;

	auto updateValue(std::string_view value) -> cmd::Result override;

	char m_value = '\0';
};

class ConVarColor final : private ConVar {
public:
	ConVarColor(std::string name, Color defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	operator const Color&() const noexcept;

private:
	auto updateValue(std::string_view value) -> cmd::Result override;

	Color m_value;
};

class ConVarHashed final : private ConVar {
public:
	ConVarHashed(std::string name, std::string_view defaultValue, Flags flags, std::string description, Callback onModified = nullptr);

	using ConVar::set;
	using ConVar::setSilent;

	[[nodiscard]] auto cvar() noexcept -> ConVar&;
	[[nodiscard]] auto cvar() const noexcept -> const ConVar&;

	[[nodiscard]] auto getHashSalt() const noexcept -> std::optional<crypto::pw::SaltView>;
	[[nodiscard]] auto getHashType() const noexcept -> crypto::pw::HashType;

	[[nodiscard]] auto verifyHash(crypto::pw::KeyView key) const noexcept -> bool;

private:
	auto updateValue(std::string_view value) -> cmd::Result override;

	crypto::FastHash m_hash{};
	crypto::pw::Salt m_salt{};
};

#define CONVAR_CALLBACK(name) \
	auto name([[maybe_unused]] ConVar& self, \
	          [[maybe_unused]] std::string_view oldVal, \
	          [[maybe_unused]] Game& game, \
	          [[maybe_unused]] GameServer* server, \
	          [[maybe_unused]] GameClient* client, \
	          [[maybe_unused]] MetaServer* metaServer, \
	          [[maybe_unused]] MetaClient* metaClient) \
		->cmd::Result

#endif
