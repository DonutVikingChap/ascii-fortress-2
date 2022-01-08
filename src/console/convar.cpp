#include "convar.hpp"

#include "../game/game.hpp"               // Game
#include "../game/server/game_server.hpp" // GameServer
#include "../utilities/algorithm.hpp"     // util::noneOf
#include "../utilities/span.hpp"          // util::Span, util::asBytes
#include "../utilities/string.hpp"        // util::join, util::stringTo, util::toString
#include "con_command.hpp"                // ConCommand
#include "process.hpp"                    // Process
#include "script.hpp"                     // Script

#include <cassert>    // assert
#include <cstdlib>    // std::byte
#include <fmt/core.h> // fmt::format

auto ConVar::all() -> std::unordered_map<std::string_view, ConVar&>& {
	static std::unordered_map<std::string_view, ConVar&> cvars;
	return cvars;
}

auto ConVar::find(std::string_view name) -> ConVar* {
	auto& cvars = ConVar::all();

	const auto it = cvars.find(name);
	return (it == cvars.end()) ? nullptr : &it->second;
}

ConVar::ConVar(Type type, std::string name, std::string defaultValue, Flags flags, std::string description, Callback onModified)
	: m_type(type)
	, m_name(std::move(name))
	, m_string(defaultValue)
	, m_flags(flags)
	, m_description(std::move(description))
	, m_defaultValue(std::move(defaultValue))
	, m_callback(onModified) {
	assert((m_flags & HASHED) == 0 || m_type == Type::HASH);
	assert((m_flags & REPLICATED) == 0 || m_type != Type::HASH);
	assert(util::noneOf(this->getName(), Script::isWhitespace));
	assert(ConVar::all().count(this->getName()) == 0);
	assert(ConCommand::all().count(this->getName()) == 0);
	ConVar::all().emplace(this->getName(), *this);
}

ConVar::~ConVar() {
	ConVar::all().erase(this->getName());
}

auto ConVar::set(std::string_view value, Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient)
	-> cmd::Result {
	return this->setValue(value, &game, server, client, metaServer, metaClient);
}

auto ConVar::setSilent(std::string_view value) -> cmd::Result {
	return this->setValue(value, nullptr, nullptr, nullptr, nullptr, nullptr);
}

auto ConVar::overrideLocalValue(std::string_view value, Game& game, GameServer* server, GameClient* client, MetaServer* metaServer,
                                MetaClient* metaClient) -> cmd::Result {
	if (!m_localValue) {
		m_localValue = m_string;
	}
	return this->set(value, game, server, client, metaServer, metaClient);
}

auto ConVar::overrideLocalValueSilent(std::string_view value) -> cmd::Result {
	if (!m_localValue) {
		m_localValue = m_string;
	}
	return this->setSilent(value);
}

auto ConVar::restoreLocalValue(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) -> cmd::Result {
	if (m_localValue) {
		auto result = this->set(*m_localValue, game, server, client, metaServer, metaClient);
		m_localValue.reset();
		return result;
	}
	return cmd::done();
}

auto ConVar::restoreLocalValueSilent() -> cmd::Result {
	if (m_localValue) {
		auto result = this->setSilent(*m_localValue);
		m_localValue.reset();
		return result;
	}
	return cmd::done();
}

auto ConVar::getRawLocalValue() const noexcept -> std::string_view {
	return (m_localValue) ? std::string_view{*m_localValue} : std::string_view{m_string};
}

auto ConVar::getString() const -> std::string {
	return ((m_flags & HASHED) != 0) ? "***HASHED***" : ((m_flags & SECRET) != 0) ? "***SECRET***" : m_string;
}

auto ConVar::formatFlags() const -> std::string {
	auto flags = std::vector<std::string>{};
	if ((m_flags & CHEAT) != 0) {
		flags.emplace_back("cheat");
	}
	if ((m_flags & READ_ONLY) != 0) {
		flags.emplace_back("read only");
	}
	if ((m_flags & INIT) != 0) {
		flags.emplace_back("init");
	}
	if ((m_flags & ARCHIVE) != 0) {
		flags.emplace_back("archive");
	}
	if ((m_flags & REPLICATED) != 0) {
		flags.emplace_back("replicated");
	}
	if ((m_flags & NOT_RUNNING_GAME_SERVER) != 0) {
		flags.emplace_back("not running game server");
	}
	if ((m_flags & NOT_RUNNING_GAME_CLIENT) != 0) {
		flags.emplace_back("not running game client");
	}
	if ((m_flags & NOT_RUNNING_META_SERVER) != 0) {
		flags.emplace_back("not running meta server");
	}
	if ((m_flags & NOT_RUNNING_META_CLIENT) != 0) {
		flags.emplace_back("not running meta client");
	}
	if ((m_flags & READ_ADMIN_ONLY) != 0) {
		flags.emplace_back("read admin only");
	}
	if ((m_flags & WRITE_ADMIN_ONLY) != 0) {
		flags.emplace_back("write admin only");
	}
	if ((m_flags & NO_RCON_WRITE) != 0) {
		flags.emplace_back("no rcon write");
	}
	if ((m_flags & NO_RCON_READ) != 0) {
		flags.emplace_back("no rcon read");
	}
	if ((m_flags & HASHED) != 0) {
		flags.emplace_back("hashed");
	}
	if ((m_flags & SECRET) != 0) {
		flags.emplace_back("secret");
	}
	return flags | util::join(", ");
}

auto ConVar::format(bool admin, bool rcon, bool defaultValue, bool limits, bool flags, bool description) const -> std::string {
	const auto allowRead = (admin || (m_flags & READ_ADMIN_ONLY) == 0) && (!rcon || (m_flags & NO_RCON_READ) == 0);
	const auto allowReadInfo = allowRead && (m_flags & HASHED) == 0 && (m_flags & SECRET) == 0;

	auto str = fmt::format("{} = \"{}\"", m_name, (allowRead) ? this->getString() : "???");
	if (m_localValue && m_string != *m_localValue && allowReadInfo) {
		str.append(fmt::format(" (local: \"{}\")", *m_localValue));
	}
	if (defaultValue && m_string != m_defaultValue && allowReadInfo) {
		str.append(fmt::format(" (default: \"{}\")", m_defaultValue));
	}

	const auto minValue = this->getMinValue();
	const auto maxValue = this->getMaxValue();

	if (limits && minValue != maxValue && allowReadInfo) {
		if (minValue < maxValue) {
			str.append(fmt::format(" (min: {:g}, max: {:g})", minValue, maxValue));
		} else {
			str.append(fmt::format(" (min: {:g})", minValue));
		}
	}

	if (flags && m_flags != NO_FLAGS) {
		str.append(fmt::format(" ({})", this->formatFlags()));
	}
	if (description) {
		str.append(fmt::format(": {}", m_description));
	}
	return str;
}

auto ConVar::getType() const noexcept -> Type {
	return m_type;
}

auto ConVar::getName() const noexcept -> std::string_view {
	return m_name;
}

auto ConVar::getRaw() const noexcept -> std::string_view {
	return m_string;
}

auto ConVar::getFlags() const noexcept -> Flags {
	return m_flags;
}

auto ConVar::getDescription() const noexcept -> std::string_view {
	return m_description;
}

auto ConVar::getDefaultValue() const noexcept -> std::string_view {
	return m_defaultValue;
}

auto ConVar::getMinValue() const noexcept -> float {
	return 0.0f;
}

auto ConVar::getMaxValue() const noexcept -> float {
	return 0.0f;
}

auto ConVar::getBool() const noexcept -> bool {
	return false;
}

auto ConVar::getInt() const noexcept -> int {
	return 0;
}

auto ConVar::getFloat() const noexcept -> float {
	return 0.0f;
}

auto ConVar::getChar() const noexcept -> char {
	return '\0';
}

auto ConVar::setString(const std::string& str) -> void {
	m_string = str;
}

auto ConVar::setString(std::string&& str) noexcept -> void {
	m_string = std::move(str);
}

auto ConVar::setValue(std::string_view value, Game* game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient)
	-> cmd::Result {
	DEBUG_MSG(Msg::CONVAR_EVENT, "Setting cvar \"{}\" to \"{}\".", this->getName(), value);
	auto oldVal = std::move(m_string);
	m_string.clear();

	auto result = this->updateValue(value);
	if (result.status == cmd::Status::ERROR_MSG) {
		m_string = std::move(oldVal);
		return result;
	}

	if (result.status == cmd::Status::NONE && game && m_callback) {
		result = m_callback(*this, oldVal, *game, server, client, metaServer, metaClient);
		if (result.status == cmd::Status::ERROR_MSG) {
			m_string = std::move(oldVal);
			return result;
		}
	}

	if ((m_flags & REPLICATED) != 0) {
		GameServer::replicate(*this);
	}

	return result;
}

ConVarString::ConVarString(std::string name, std::string defaultValue, Flags flags, std::string description, Callback onModified)
	: ConVar(Type::STRING, std::move(name), std::move(defaultValue), flags, std::move(description), onModified) {}

auto ConVarString::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarString::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarString::operator std::string_view() const noexcept {
	return this->getRaw();
}

auto ConVarString::begin() const noexcept -> std::string_view::const_iterator {
	return this->getRaw().begin();
}

auto ConVarString::end() const noexcept -> std::string_view::const_iterator {
	return this->getRaw().end();
}

auto ConVarString::cbegin() const noexcept -> std::string_view::const_iterator {
	return this->getRaw().cbegin();
}

auto ConVarString::cend() const noexcept -> std::string_view::const_iterator {
	return this->getRaw().cend();
}

auto ConVarString::rbegin() const noexcept -> std::string_view::const_reverse_iterator {
	return this->getRaw().rbegin();
}

auto ConVarString::rend() const noexcept -> std::string_view::const_reverse_iterator {
	return this->getRaw().rend();
}

auto ConVarString::crbegin() const noexcept -> std::string_view::const_reverse_iterator {
	return this->getRaw().crbegin();
}

auto ConVarString::crend() const noexcept -> std::string_view::const_reverse_iterator {
	return this->getRaw().crend();
}

auto ConVarString::operator[](std::size_t i) const -> const char& {
	return this->getRaw()[i];
}

auto ConVarString::at(std::size_t i) const -> const char& {
	return this->getRaw().at(i);
}

auto ConVarString::front() const noexcept -> const char& {
	return this->getRaw().front();
}

auto ConVarString::back() const noexcept -> const char& {
	return this->getRaw().back();
}

auto ConVarString::data() const noexcept -> const char* {
	return this->getRaw().data();
}

auto ConVarString::size() const noexcept -> std::size_t {
	return this->getRaw().size();
}

auto ConVarString::length() const noexcept -> std::size_t {
	return this->getRaw().length();
}

auto ConVarString::max_size() const noexcept -> std::size_t {
	return this->getRaw().max_size();
}

auto ConVarString::empty() const noexcept -> bool {
	return this->getRaw().empty();
}

auto ConVarString::copy(char* destination, std::size_t count, std::size_t pos) const -> std::size_t {
	return this->getRaw().copy(destination, count, pos);
}

auto ConVarString::substr(std::size_t pos, std::size_t count) const -> std::string_view {
	return this->getRaw().substr(pos, count);
}

auto ConVarString::compare(std::string_view str) const noexcept -> int {
	return this->getRaw().compare(str);
}

auto ConVarString::compare(std::size_t pos, std::size_t count, std::string_view str) const noexcept -> int {
	return this->getRaw().compare(pos, count, str);
}

auto ConVarString::compare(std::size_t pos1, std::size_t count1, std::string_view str, std::size_t pos2, std::size_t count2) const noexcept -> int {
	return this->getRaw().compare(pos1, count1, str, pos2, count2);
}

auto ConVarString::compare(const char* str) const noexcept -> int {
	return this->getRaw().compare(str);
}

auto ConVarString::compare(std::size_t pos, std::size_t count, const char* str) const noexcept -> int {
	return this->getRaw().compare(pos, count, str);
}

auto ConVarString::compare(std::size_t pos1, std::size_t count1, const char* str, std::size_t pos2, std::size_t count2) const noexcept -> int {
	return this->getRaw().compare(pos1, count1, str, pos2, count2);
}

auto ConVarString::find(std::string_view str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find(str, pos);
}

auto ConVarString::find(char ch, std::size_t pos) const -> std::size_t {
	return this->getRaw().find(ch, pos);
}

auto ConVarString::find(const char* str, std::size_t pos, std::size_t count) const -> std::size_t {
	return this->getRaw().find(str, pos, count);
}

auto ConVarString::find(const char* str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find(str, pos);
}

auto ConVarString::rfind(std::string_view str, std::size_t pos) const -> std::size_t {
	return this->getRaw().rfind(str, pos);
}

auto ConVarString::rfind(char ch, std::size_t pos) const -> std::size_t {
	return this->getRaw().rfind(ch, pos);
}

auto ConVarString::rfind(const char* str, std::size_t pos, std::size_t count) const -> std::size_t {
	return this->getRaw().rfind(str, pos, count);
}

auto ConVarString::rfind(const char* str, std::size_t pos) const -> std::size_t {
	return this->getRaw().rfind(str, pos);
}

auto ConVarString::find_first_of(std::string_view str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_first_of(str, pos);
}

auto ConVarString::find_first_of(char ch, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_first_of(ch, pos);
}

auto ConVarString::find_first_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t {
	return this->getRaw().find_first_of(str, pos, count);
}

auto ConVarString::find_first_of(const char* str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_first_of(str, pos);
}

auto ConVarString::find_last_of(std::string_view str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_last_of(str, pos);
}

auto ConVarString::find_last_of(char ch, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_last_of(ch, pos);
}

auto ConVarString::find_last_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t {
	return this->getRaw().find_last_of(str, pos, count);
}

auto ConVarString::find_last_of(const char* str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_last_of(str, pos);
}

auto ConVarString::find_first_not_of(std::string_view str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_first_not_of(str, pos);
}

auto ConVarString::find_first_not_of(char ch, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_first_not_of(ch, pos);
}

auto ConVarString::find_first_not_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t {
	return this->getRaw().find_first_not_of(str, pos, count);
}

auto ConVarString::find_first_not_of(const char* str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_first_not_of(str, pos);
}

auto ConVarString::find_last_not_of(std::string_view str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_last_not_of(str, pos);
}

auto ConVarString::find_last_not_of(char ch, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_last_not_of(ch, pos);
}

auto ConVarString::find_last_not_of(const char* str, std::size_t pos, std::size_t count) const -> std::size_t {
	return this->getRaw().find_last_not_of(str, pos, count);
}

auto ConVarString::find_last_not_of(const char* str, std::size_t pos) const -> std::size_t {
	return this->getRaw().find_last_not_of(str, pos);
}

auto operator==(const ConVarString& lhs, std::string_view rhs) noexcept -> bool {
	return std::string_view{lhs} == rhs;
}

auto operator!=(const ConVarString& lhs, std::string_view rhs) noexcept -> bool {
	return std::string_view{lhs} != rhs;
}

auto operator<(const ConVarString& lhs, std::string_view rhs) noexcept -> bool {
	return std::string_view{lhs} < rhs;
}

auto operator<=(const ConVarString& lhs, std::string_view rhs) noexcept -> bool {
	return std::string_view{lhs} <= rhs;
}

auto operator>(const ConVarString& lhs, std::string_view rhs) noexcept -> bool {
	return std::string_view{lhs} > rhs;
}

auto operator>=(const ConVarString& lhs, std::string_view rhs) noexcept -> bool {
	return std::string_view{lhs} >= rhs;
}

auto operator==(std::string_view lhs, const ConVarString& rhs) noexcept -> bool {
	return lhs == std::string_view{rhs};
}

auto operator!=(std::string_view lhs, const ConVarString& rhs) noexcept -> bool {
	return lhs != std::string_view{rhs};
}

auto operator<(std::string_view lhs, const ConVarString& rhs) noexcept -> bool {
	return lhs < std::string_view{rhs};
}

auto operator<=(std::string_view lhs, const ConVarString& rhs) noexcept -> bool {
	return lhs <= std::string_view{rhs};
}

auto operator>(std::string_view lhs, const ConVarString& rhs) noexcept -> bool {
	return lhs > std::string_view{rhs};
}

auto operator>=(std::string_view lhs, const ConVarString& rhs) noexcept -> bool {
	return lhs >= std::string_view{rhs};
}

auto ConVarString::getBool() const noexcept -> bool {
	return !this->empty();
}

auto ConVarString::getInt() const noexcept -> int {
	if (const auto value = util::stringTo<int>(this->getRaw())) {
		return *value;
	}
	return 0;
}

auto ConVarString::getFloat() const noexcept -> float {
	if (const auto value = util::stringTo<float>(this->getRaw())) {
		return *value;
	}
	return 0.0f;
}

auto ConVarString::getChar() const noexcept -> char {
	if (const auto& value = this->getRaw(); !value.empty()) {
		return value.front();
	}
	return '\0';
}

auto ConVarString::updateValue(std::string_view value) -> cmd::Result {
	this->setString(std::string{value});
	return cmd::done();
}

ConVarBool::ConVarBool(std::string name, bool defaultValue, Flags flags, std::string description, Callback onModified)
	: ConVar(Type::BOOL, std::move(name), (defaultValue) ? "1" : "0", flags, std::move(description), onModified)
	, m_value(defaultValue) {}

auto ConVarBool::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarBool::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarBool::operator const bool&() const noexcept {
	return m_value;
}

auto ConVarBool::getMinValue() const noexcept -> float {
	return 0.0f;
}

auto ConVarBool::getMaxValue() const noexcept -> float {
	return 1.0f;
}

auto ConVarBool::getBool() const noexcept -> bool {
	return m_value;
}

auto ConVarBool::getInt() const noexcept -> int {
	return (m_value) ? 1 : 0;
}

auto ConVarBool::getFloat() const noexcept -> float {
	return (m_value) ? 1.0f : 0.0f;
}

auto ConVarBool::getChar() const noexcept -> char {
	return (m_value) ? '1' : '0';
}

auto ConVarBool::updateValue(std::string_view value) -> cmd::Result {
	if (value.size() == 1) {
		if (value.front() == '0') {
			m_value = false;
			this->setString(std::string{value});
			return cmd::done();
		}

		if (value.front() == '1') {
			m_value = true;
			this->setString(std::string{value});
			return cmd::done();
		}
	}
	return cmd::error("{}: Couldn't parse \"{}\". Value must be 1 or 0.", this->getName(), value);
}

ConVarInt::ConVarInt(std::string name, int defaultValue, Flags flags, std::string description, Callback onModified)
	: ConVar(Type::INT, std::move(name), util::toString(defaultValue), flags, std::move(description), onModified)
	, m_value(defaultValue) {}

auto ConVarInt::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarInt::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarInt::operator const int&() const noexcept {
	return m_value;
}

auto ConVarInt::getBool() const noexcept -> bool {
	return m_value != 0;
}

auto ConVarInt::getInt() const noexcept -> int {
	return m_value;
}

auto ConVarInt::getFloat() const noexcept -> float {
	return static_cast<float>(m_value);
}

auto ConVarInt::getChar() const noexcept -> char {
	return static_cast<char>(m_value);
}

auto ConVarInt::updateValue(std::string_view value) -> cmd::Result {
	if (const auto val = util::stringTo<int>(value)) {
		m_value = *val;
		this->setString(std::string{value});
		return cmd::done();
	}
	return cmd::error("{}: Couldn't parse \"{}\". Value must be an integer.", this->getName(), value);
}

ConVarIntMinMax::ConVarIntMinMax(std::string name, int defaultValue, Flags flags, std::string description, int minValue, int maxValue, Callback onModified)
	: ConVar(Type::INT, std::move(name), util::toString(defaultValue), flags, std::move(description), onModified)
	, m_value(defaultValue)
	, m_minValue(minValue)
	, m_maxValue(maxValue) {}

auto ConVarIntMinMax::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarIntMinMax::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarIntMinMax::operator const int&() const noexcept {
	return m_value;
}

auto ConVarIntMinMax::getMinValue() const noexcept -> float {
	return static_cast<float>(m_minValue);
}

auto ConVarIntMinMax::getMaxValue() const noexcept -> float {
	return static_cast<float>(m_maxValue);
}

auto ConVarIntMinMax::getBool() const noexcept -> bool {
	return m_value != 0;
}

auto ConVarIntMinMax::getInt() const noexcept -> int {
	return m_value;
}

auto ConVarIntMinMax::getFloat() const noexcept -> float {
	return static_cast<float>(m_value);
}

auto ConVarIntMinMax::getChar() const noexcept -> char {
	return static_cast<char>(m_value);
}

auto ConVarIntMinMax::updateValue(std::string_view value) -> cmd::Result {
	if (const auto val = util::stringTo<int>(value)) {
		if (*val < m_minValue) {
			return cmd::error("{}: {} is less than the minimum value ({}).", this->getName(), *val, m_minValue);
		}
		if (*val > m_maxValue && m_minValue < m_maxValue) {
			return cmd::error("{}: {} is greater than the maximum value ({}).", this->getName(), *val, m_maxValue);
		}
		m_value = *val;
		this->setString(std::string{value});
		return cmd::done();
	}
	return cmd::error("{}: Couldn't parse \"{}\". Value must be an integer.", this->getName(), value);
}

ConVarFloat::ConVarFloat(std::string name, float defaultValue, Flags flags, std::string description, Callback onModified)
	: ConVar(Type::FLOAT, std::move(name), util::toString(defaultValue), flags, std::move(description), onModified)
	, m_value(defaultValue) {}

auto ConVarFloat::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarFloat::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarFloat::operator const float&() const noexcept {
	return m_value;
}

auto ConVarFloat::getBool() const noexcept -> bool {
	return m_value != 0.0f;
}

auto ConVarFloat::getInt() const noexcept -> int {
	return static_cast<int>(m_value);
}

auto ConVarFloat::getFloat() const noexcept -> float {
	return m_value;
}

auto ConVarFloat::getChar() const noexcept -> char {
	return static_cast<char>(m_value);
}

auto ConVarFloat::updateValue(std::string_view value) -> cmd::Result {
	if (const auto val = util::stringTo<float>(value)) {
		m_value = *val;
		this->setString(std::string{value});
		return cmd::done();
	}
	return cmd::error("{}: Couldn't parse \"{}\". Value must be a number.", this->getName(), value);
}

ConVarFloatMinMax::ConVarFloatMinMax(std::string name, float defaultValue, Flags flags, std::string description, float minValue,
                                     float maxValue, Callback onModified)
	: ConVar(Type::FLOAT, std::move(name), util::toString(defaultValue), flags, std::move(description), onModified)
	, m_value(defaultValue)
	, m_minValue(minValue)
	, m_maxValue(maxValue) {}

auto ConVarFloatMinMax::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarFloatMinMax::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarFloatMinMax::operator const float&() const noexcept {
	return m_value;
}

auto ConVarFloatMinMax::getMinValue() const noexcept -> float {
	return m_minValue;
}

auto ConVarFloatMinMax::getMaxValue() const noexcept -> float {
	return m_maxValue;
}

auto ConVarFloatMinMax::getBool() const noexcept -> bool {
	return m_value != 0.0f;
}

auto ConVarFloatMinMax::getInt() const noexcept -> int {
	return static_cast<int>(m_value);
}

auto ConVarFloatMinMax::getFloat() const noexcept -> float {
	return m_value;
}

auto ConVarFloatMinMax::getChar() const noexcept -> char {
	return static_cast<char>(m_value);
}

auto ConVarFloatMinMax::updateValue(std::string_view value) -> cmd::Result {
	if (const auto val = util::stringTo<float>(value)) {
		if (*val < m_minValue) {
			return cmd::error("{}: {} is less than the minimum value ({}).", this->getName(), *val, m_minValue);
		}
		if (*val > m_maxValue && m_minValue < m_maxValue) {
			return cmd::error("{}: {} is greater than the maximum value ({}).", this->getName(), *val, m_maxValue);
		}
		m_value = *val;
		this->setString(std::string{value});
		return cmd::done();
	}
	return cmd::error("{}: Couldn't parse \"{}\". Value must be a number.", this->getName(), value);
}

ConVarChar::ConVarChar(std::string name, char defaultValue, Flags flags, std::string description, Callback onModified)
	: ConVar(Type::CHAR, std::move(name), std::string({defaultValue}), flags, std::move(description), onModified)
	, m_value(defaultValue) {}

auto ConVarChar::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarChar::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarChar::operator const char&() const noexcept {
	return m_value;
}

auto ConVarChar::getBool() const noexcept -> bool {
	return m_value != '\0';
}

auto ConVarChar::getInt() const noexcept -> int {
	return static_cast<int>(m_value);
}

auto ConVarChar::getFloat() const noexcept -> float {
	return static_cast<float>(m_value);
}

auto ConVarChar::getChar() const noexcept -> char {
	return m_value;
}

auto ConVarChar::updateValue(std::string_view value) -> cmd::Result {
	if (value.size() != 1) {
		return cmd::error("{}: Couldn't parse \"{}\". Value must be a single character.", this->getName(), value);
	}
	m_value = value.front();
	this->setString(std::string{value});
	return cmd::done();
}

ConVarColor::ConVarColor(std::string name, Color defaultValue, Flags flags, std::string description, Callback onModified)
	: ConVar(Type::COLOR, std::move(name), defaultValue.getString(), flags, std::move(description), onModified)
	, m_value(defaultValue) {}

auto ConVarColor::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarColor::cvar() const noexcept -> const ConVar& {
	return *this;
}

ConVarColor::operator const Color&() const noexcept {
	return m_value;
}

auto ConVarColor::updateValue(std::string_view value) -> cmd::Result {
	if (const auto val = Color::parse(value)) {
		m_value = *val;
		this->setString(m_value.getString());
		return cmd::done();
	}
	return cmd::error("{}: Couldn't parse \"{}\". Value must be a color.", this->getName(), value);
}

ConVarHashed::ConVarHashed(std::string name, std::string_view defaultValue, Flags flags, std::string description, Callback onModified)
	: ConVar(Type::HASH, std::move(name), "", flags | HASHED, std::move(description), onModified) {
	this->updateValue(defaultValue);
}

auto ConVarHashed::cvar() noexcept -> ConVar& {
	return *this;
}

auto ConVarHashed::cvar() const noexcept -> const ConVar& {
	return *this;
}

auto ConVarHashed::getHashSalt() const noexcept -> std::optional<crypto::pw::SaltView> {
	return m_salt;
}

auto ConVarHashed::getHashType() const noexcept -> crypto::pw::HashType { // NOLINT(readability-convert-member-functions-to-static)
	return crypto::pw::HashType::FAST;
}

auto ConVarHashed::verifyHash(crypto::pw::KeyView key) const noexcept -> bool {
	if (!crypto::init()) {
		return false;
	}
	return crypto::verifyFastHash(m_hash, util::asBytes(util::Span{key}));
}

auto ConVarHashed::updateValue(std::string_view value) -> cmd::Result {
	if (!crypto::init()) {
		return cmd::error("{}: Failed to initialize crypto library!", this->getName());
	}
	m_hash.fill(0);
	crypto::pw::generateSalt(m_salt);

	auto key = crypto::pw::Key{};
	if (!crypto::pw::deriveKey(key, m_salt, value, this->getHashType())) {
		return cmd::error("{}: Failed to derive key!", this->getName());
	}
	if (!crypto::fastHash(m_hash, util::asBytes(util::Span{key}))) {
		return cmd::error("{}: Failed to hash key!", this->getName());
	}
	return cmd::done();
}
