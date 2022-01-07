#include "script.hpp"

#include "../utilities/algorithm.hpp" // util::subview, util::anyOf, util::transform, util::move
#include "../utilities/string.hpp"    // util::join

#include <cassert>    // assert
#include <fmt/core.h> // fmt::format
#include <iterator>   // std::back_inserter

class ScriptParser final {
public:
	[[nodiscard]] static auto parseScript(std::string_view script) -> Script {
		return ScriptParser{script}.parseScript();
	}

private:
	explicit ScriptParser(std::string_view script) noexcept
		: m_it(script.begin())
		, m_end(script.end()) {}

	[[nodiscard]] auto parseScript() -> Script {
		auto commands = Script{};
		while (!this->atEnd()) {
			if (this->checkWhitespace()) {
				this->skipWhitespace();
			} else if (this->checkComment()) {
				this->skipComment();
			} else if (this->checkCommandSeparator()) {
				this->skipCommandSeparator();
				this->endCommand(commands);
			} else if (this->checkPipe()) {
				this->skipPipe();
				if (this->hasArgument()) {
					this->makePipe();
					this->endCommand(commands);
				}
			} else if (this->peek() == '\\' && this->peek(1) == '\n') {
				this->advance(2);
			} else {
				this->addArgument();
				switch (this->peek()) {
					case '\"': this->readQuote(); break;
					case '(':
						this->makeExpression();
						this->readParentheses();
						break;
					case '{': this->readCurlyBraces(); break;
					default: this->readToken(); break;
				}
			}
		}
		this->endCommand(commands);
		return commands;
	}

	auto readEscapeSequence() -> void {
		this->advance();
		if (!this->atEnd()) {
			switch (this->peek()) {
				case 't': this->addCharacter('\t'); break;
				case 'r': this->addCharacter('\r'); break;
				case 'n': this->addCharacter('\n'); break;
				case '0': this->addCharacter('\0'); break;
				case 'x':
					if (!this->atEnd(2)) {
						this->advance();
						const auto highNibble = this->peek();
						this->advance();
						const auto lowNibble = this->peek();
						this->addCharacter(static_cast<char>((Script::hexValue(highNibble) << 4) | Script::hexValue(lowNibble)));
						break;
					}
					[[fallthrough]];
				default: this->addCharacter(this->peek()); break;
			}
			this->advance();
		}
	}

	auto readEscapeSequenceVerbatim() -> void {
		this->addCharacter('\\');
		this->advance();
		if (!this->atEnd()) {
			if (this->peek() == 'x' && !this->atEnd(2)) {
				this->addCharacter(this->peek());
				this->advance();
				this->addCharacter(this->peek());
				this->advance();
			} else {
				this->addCharacter(this->peek());
				this->advance();
			}
		}
	}

	auto readQuote() -> void {
		this->advance();
		while (!this->atEnd()) {
			if (this->peek() == '\"') {
				this->advance();
				break;
			}

			if (this->peek() == '\\') {
				this->readEscapeSequence();
			} else {
				this->addCharacter(this->peek());
				this->advance();
			}
		}
	}

	auto readQuoteVerbatim() -> void {
		this->addCharacter('\"');
		this->advance();
		while (!this->atEnd()) {
			if (this->peek() == '\"') {
				this->addCharacter('\"');
				this->advance();
				break;
			}

			if (this->peek() == '\\') {
				this->readEscapeSequenceVerbatim();
			} else {
				this->addCharacter(this->peek());
				this->advance();
			}
		}
	}

	auto readToken() -> void {
		if (this->peek() == '$') {
			this->makeExpression();
			this->advance();
		}

		while (!this->atEnd()) {
			if (this->checkWhitespace() || this->checkComment() || this->checkCommandSeparator() || this->checkPipe()) {
				break;
			}

			if (this->checkExpansion()) {
				this->skipExpansion();
				if (this->currentArgument().value.empty()) {
					this->currentArgument().value = "...";
				} else {
					this->makeExpansion();
					break;
				}
			} else {
				switch (this->peek()) {
					case '\\': this->readEscapeSequence(); break;
					case '(': {
						this->makeExpression();
						const auto i = this->currentArgument().value.size();
						this->readParentheses();
						if (this->currentArgument().value.size() != i) {
							this->currentArgument().value.insert(this->currentArgument().value.begin() + static_cast<std::ptrdiff_t>(i), ' ');
						}
						return;
					}
					case '{': this->readCurlyBraces(); return;
					case '\"': this->readQuote(); break;
					default:
						this->addCharacter(this->peek());
						this->advance();
						break;
				}
			}
		}
	}

	auto readBracket(std::size_t parenLevel, std::size_t braceLevel) -> void {
		this->advance();
		while (!this->atEnd()) {
			switch (this->peek()) {
				case '\"': this->readQuoteVerbatim(); continue;
				case '(': ++parenLevel; break;
				case '{': ++braceLevel; break;
				case ')':
					if (--parenLevel == 0 && braceLevel == 0) {
						this->advance();
						return;
					}
					break;
				case '}':
					if (--braceLevel == 0 && parenLevel == 0) {
						this->advance();
						return;
					}
					break;
			}
			this->addCharacter(this->peek());
			this->advance();
		}
	}

	auto readParentheses() -> void {
		this->readBracket(1, 0);
	}

	auto readCurlyBraces() -> void {
		this->readBracket(0, 1);
	}

	[[nodiscard]] auto atEnd() const noexcept -> bool {
		return m_it == m_end;
	}

	[[nodiscard]] auto atEnd(std::ptrdiff_t offset) const noexcept -> bool {
		return m_end - m_it <= offset;
	}

	[[nodiscard]] auto peek() const noexcept -> char {
		assert(m_it < m_end);
		return *m_it;
	}

	[[nodiscard]] auto peek(std::ptrdiff_t offset) const noexcept -> char {
		return this->atEnd(offset) ? '\0' : *(m_it + offset);
	}

	auto advance() noexcept -> void {
		assert(!this->atEnd());
		++m_it;
	}

	auto advance(std::ptrdiff_t steps) noexcept -> void {
		assert(!this->atEnd(steps - 1));
		m_it += steps;
	}

	auto addArgument() -> void {
		m_command.emplace_back();
	}

	auto addArgument(Script::Argument arg) -> void {
		m_command.push_back(std::move(arg));
	}

	auto addCharacter(char ch) -> void {
		this->currentArgument().value.push_back(ch);
	}

	auto makeExpression() noexcept -> void {
		this->currentArgument().flags |= Script::Argument::EXEC;
	}

	auto makeExpansion() noexcept -> void {
		this->currentArgument().flags |= Script::Argument::EXPAND;
	}

	auto makePipe() noexcept -> void {
		this->currentArgument().flags |= Script::Argument::PIPE;
	}

	[[nodiscard]] auto hasArgument() noexcept -> bool {
		return !m_command.empty();
	}

	[[nodiscard]] auto currentArgument() noexcept -> Script::Argument& {
		assert(!m_command.empty());
		return m_command.back();
	}

	[[nodiscard]] auto currentArgument() const noexcept -> const Script::Argument& {
		assert(!m_command.empty());
		return m_command.back();
	}

	auto endCommand(Script& commands) -> void {
		if (!m_command.empty()) {
			commands.push_back(std::move(m_command));
			m_command.clear();
		}
	}

	[[nodiscard]] auto checkWhitespace() const noexcept -> bool {
		return Script::isWhitespace(this->peek());
	}

	auto skipWhitespace() noexcept -> void {
		this->advance();
		while (!this->atEnd() && this->checkWhitespace()) {
			this->advance();
		}
	}

	[[nodiscard]] auto checkCommandSeparator() const noexcept -> bool {
		return Script::isCommandSeparator(this->peek());
	}

	auto skipCommandSeparator() noexcept -> void {
		this->advance();
	}

	[[nodiscard]] auto checkPipe() const noexcept -> bool {
		return this->peek() == '|';
	}

	auto skipPipe() noexcept -> void {
		this->advance();
	}

	[[nodiscard]] auto checkExpansion() const noexcept -> bool {
		return this->peek() == '.' && this->peek(1) == '.' && this->peek(2) == '.';
	}

	auto skipExpansion() noexcept -> void {
		this->advance(3);
	}

	[[nodiscard]] auto checkComment() const noexcept -> bool {
		return this->peek() == '/' && this->peek(1) == '/';
	}

	auto skipComment() noexcept -> void {
		this->advance(2);
		this->skipLine();
	}

	auto skipLine() noexcept -> void {
		while (!this->atEnd() && this->peek() != '\n') {
			this->advance();
		}
	}

	std::string_view::const_iterator m_it;
	std::string_view::const_iterator m_end;
	Script::Command m_command;
};

auto Script::parse(std::string_view script) -> Script {
	return ScriptParser::parseScript(script);
}

auto Script::escapedString(std::string_view str) -> std::string {
	auto arg = std::string{};
	arg.reserve(str.size() + 2);
	arg.push_back('\"');
	for (const auto ch : str) {
		if (Script::isPrintableChar(ch) && ch != '\"' && ch != '\\') {
			arg.push_back(ch);
		} else {
			arg.push_back('\\');
			switch (ch) {
				case '\"': arg.push_back('\"'); break;
				case '\\': arg.push_back('\\'); break;
				case '\t': arg.push_back('t'); break;
				case '\r': arg.push_back('r'); break;
				case '\n': arg.push_back('n'); break;
				case '\0': arg.push_back('0'); break;
				default:
					arg.push_back('x');
					arg.push_back(Script::HEX_DIGITS[(ch >> 4) & 0xF]); // High nibble.
					arg.push_back(Script::HEX_DIGITS[ch & 0xF]);        // Low nibble.
					break;
			}
		}
	}
	arg.push_back('\"');
	return arg;
}

auto Script::argumentString(const Argument& argument) -> std::string {
	if ((argument.flags & Argument::PIPE) != 0) {
		return std::string{"|"};
	}
	auto str = std::string{argument.value};
	if (util::anyOf(str, Script::isWhitespace)) {
		str = Script::escapedString(str);
	}
	if ((argument.flags & Argument::EXPAND) != 0) {
		str.append("...");
	}
	if ((argument.flags & Argument::EXEC) != 0) {
		return fmt::format("${}", str);
	}
	return str;
}

auto Script::commandString(const Command& command) -> std::string {
	return command | util::transform(Script::argumentString) | util::join(' ');
}

auto Script::scriptString(const Script& commands) -> std::string {
	return commands | util::transform(Script::commandString) | util::join("; ");
}

auto Script::command(std::initializer_list<std::string_view> init) -> Script::Command {
	auto command = Command{};
	command.reserve(init.size());
	for (const auto& value : init) {
		command.emplace_back(std::string{value});
	}
	return command;
}

auto Script::subCommand(Command command, std::size_t offset, std::size_t count) -> Script::Command {
	assert(offset <= command.size());
	assert(offset + count <= command.size());
	auto subCommand = Command{};
	subCommand.reserve(count);
	util::move(util::subview(command, offset, count), std::back_inserter(subCommand));
	return subCommand;
}

auto Script::subCommand(Command command, std::size_t offset) -> Script::Command {
	assert(offset <= command.size());
	auto subCommand = Command{};
	subCommand.reserve(command.size() - offset);
	util::move(util::subview(command, offset), std::back_inserter(subCommand));
	return subCommand;
}
