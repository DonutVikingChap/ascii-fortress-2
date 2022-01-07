#ifndef AF2_GUI_CANVAS_HPP
#define AF2_GUI_CANVAS_HPP

#include "../game/data/color.hpp"       // Color
#include "../game/data/vector.hpp"      // Vec2
#include "../utilities/span.hpp"        // util::Span
#include "../utilities/tile_matrix.hpp" // util::TileMatrix
#include "cmd_button.hpp"               // gui::CmdButton
#include "cmd_checkbox.hpp"             // gui::CmdCheckbox
#include "cmd_dropdown.hpp"             // gui::CmdDropdown
#include "cmd_input.hpp"                // gui::CmdInput
#include "cmd_slider.hpp"               // gui::CmdSlider
#include "menu.hpp"                     // gui::Menu

#include <SDL.h>       // SDL_...
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <limits>      // std::numeric_limits
#include <memory>      // std::unique_ptr, std::make_unique
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::forward
#include <variant>     // std::variant
#include <vector>      // std::vector

class Game;
class VirtualMachine;
class Process;
struct Environment;
class CharWindow;

namespace gui {

class Element;

class Canvas final {
public:
	using Id = int;

	struct ButtonInfoView final {
		Id id = 0;
		Vec2 position{};
		Vec2 size{};
		Color color{};
		std::string_view text{};
		bool activated = false;
	};

	struct InputInfoView final {
		Id id = 0;
		Vec2 position{};
		Vec2 size{};
		Color color{};
		std::string_view text{};
		bool activated = false;
	};

	struct SliderInfoView final {
		Id id = 0;
		Vec2 position{};
		Vec2 size{};
		Color color{};
		bool activated = false;
	};

	struct CheckboxInfoView final {
		Id id = 0;
		Vec2 position{};
		Vec2 size{};
		Color color{};
		bool activated = false;
	};

	struct DropdownInfoView final {
		Id id = 0;
		Vec2 position{};
		Vec2 size{};
		Color color{};
		bool activated = false;
	};

	struct ScreenInfoView final {
		Id id = 0;
		Vec2 position{};
		Vec2 size{};
		Color color{};
	};

	struct TextInfoView final {
		Id id = 0;
		Vec2 position{};
		Color color{};
		std::string_view text{};
	};

	using ElementInfoView = std::variant<ButtonInfoView, InputInfoView, SliderInfoView, CheckboxInfoView, DropdownInfoView, ScreenInfoView, TextInfoView>;

	struct MenuInfoView final {
		std::vector<Id> ids{};
		bool activated = false;
		std::optional<Id> activeId{};
	};

	Canvas(Game& game, VirtualMachine& vm);
	~Canvas() = default;

	Canvas(const Canvas&) = delete;
	Canvas(Canvas&&) = delete;

	auto operator=(const Canvas&) -> Canvas& = delete;
	auto operator=(Canvas&&) -> Canvas& = delete;

	auto clear() noexcept -> void;

	[[nodiscard]] auto isClear() const noexcept -> bool;

	[[nodiscard]] auto addButton(Id id, Vec2 position, Vec2 size, Color color, std::string text, std::shared_ptr<Environment> env,
								 std::shared_ptr<Process> process, std::string_view command) -> bool;
	[[nodiscard]] auto addInput(Id id, Vec2 position, Vec2 size, Color color, std::string text, std::shared_ptr<Environment> env,
								std::shared_ptr<Process> process, std::string_view command,
								std::size_t maxLength = std::numeric_limits<std::size_t>::max(), bool isPrivate = false,
								bool replaceMode = false) -> bool;
	[[nodiscard]] auto addSlider(Id id, Vec2 position, Vec2 size, Color color, float value, float delta, std::shared_ptr<Environment> env,
								 std::shared_ptr<Process> process, std::string_view command) -> bool;
	[[nodiscard]] auto addCheckbox(Id id, Vec2 position, Vec2 size, Color color, bool value, std::shared_ptr<Environment> env,
								   std::shared_ptr<Process> process, std::string_view command) -> bool;
	[[nodiscard]] auto addDropdown(Id id, Vec2 position, Vec2 size, Color color, std::vector<std::string> options, std::size_t selectedOptionIndex,
								   std::shared_ptr<Environment> env, std::shared_ptr<Process> process, std::string_view command) -> bool;
	[[nodiscard]] auto addScreen(Id id, Vec2 position, Color color, util::TileMatrix<char> screen) -> bool;
	[[nodiscard]] auto addText(Id id, Vec2 position, Color color, std::string text) -> bool;

	[[nodiscard]] auto pushMenu(util::Span<const Id> ids, const std::shared_ptr<Environment>& env, const std::shared_ptr<Process>& process,
								std::string_view selectNoneCommand, std::string_view escapeCommand, std::string_view directionCommand,
								std::string_view clickCommand, std::string_view scrollCommand, std::string_view hoverCommand) -> bool;

	[[nodiscard]] auto hasMenu() const noexcept -> bool;

	[[nodiscard]] auto menuStackSize() const noexcept -> std::size_t;

	auto popMenu() noexcept -> bool;

	[[nodiscard]] auto hasElement(Id id) const -> bool;

	auto removeElement(Id id) -> bool;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void;

	auto update(float deltaTime) -> void;

	auto draw(CharWindow& charWindow) -> void;

	[[nodiscard]] auto isElementActivated(Id id) const -> bool;

	auto activateElement(Id id) -> bool;
	auto deactivateElement(Id id) -> bool;

	auto deactivate() -> void;

	[[nodiscard]] auto getElementText(Id id) const -> std::optional<std::string>;
	[[nodiscard]] auto getElementColor(Id id) const -> std::optional<Color>;
	[[nodiscard]] auto getElementValue(Id id) const -> std::optional<float>;

	auto setElementText(Id id, std::string str) -> bool;
	auto setElementColor(Id id, Color color) -> bool;
	auto setElementValue(Id id, float value) -> bool;

	[[nodiscard]] auto getScreenChar(Id id, std::size_t x, std::size_t y, char defaultVal) const -> std::optional<char>;

	auto setScreenChar(Id id, std::size_t x, std::size_t y, char ch) -> bool;

	[[nodiscard]] auto getElementInfo() const -> std::vector<ElementInfoView>;
	[[nodiscard]] auto getMenuInfo() const -> std::vector<MenuInfoView>;
	[[nodiscard]] auto getElementIds() const -> std::vector<Id>;

private:
	using Button = std::unique_ptr<CmdButton>;
	using Input = std::unique_ptr<CmdInput>;
	using Slider = std::unique_ptr<CmdSlider>;
	using Checkbox = std::unique_ptr<CmdCheckbox>;
	using Dropdown = std::unique_ptr<CmdDropdown>;

	struct Screen final {
		Screen(Vec2 position, Color color, util::TileMatrix<char> screen)
			: position(position)
			, color(color)
			, screen(std::move(screen)) {}

		Vec2 position;
		Color color;
		util::TileMatrix<char> screen;
	};

	struct Text final {
		Text(Vec2 position, Color color, std::string text)
			: position(position)
			, color(color)
			, text(std::move(text)) {}

		Vec2 position;
		Color color;
		std::string text;
	};

	struct Item final {
		Id id{};
		std::variant<Button, Input, Slider, Checkbox, Dropdown, Screen, Text> item{};
	};

	template <typename T, typename... Args>
	[[nodiscard]] auto addItem(Id id, Args&&... args) -> bool {
		if (this->hasElement(id)) {
			return false;
		}

		m_modified = true;

		m_items.emplace_back();
		m_items.back().id = id;
		m_items.back().item.emplace<T>(std::forward<Args>(args)...);
		return true;
	}

	[[nodiscard]] auto findItem(Id id) -> std::vector<Item>::iterator;
	[[nodiscard]] auto findItem(Id id) const -> std::vector<Item>::const_iterator;

	[[nodiscard]] auto findId(const Element& element) const -> std::optional<Id>;

	Game& m_game;
	VirtualMachine& m_vm;
	std::vector<Item> m_items{};
	std::vector<gui::Menu> m_menuStack{};
	bool m_modified = false;
};

} // namespace gui

#endif
