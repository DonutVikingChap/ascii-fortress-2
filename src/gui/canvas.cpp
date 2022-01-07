#include "canvas.hpp"

#include "../console/script.hpp"      // Script
#include "../game/game.hpp"           // Game
#include "../utilities/algorithm.hpp" // util::findIf, util::anyOf, util::collect, util::transform
#include "../utilities/match.hpp"     // util::match
#include "../utilities/string.hpp"    // util::toString
#include "element.hpp"                // gui::Element

#include <fmt/core.h> // fmt::format

namespace gui {

Canvas::Canvas(Game& game, VirtualMachine& vm)
	: m_game(game)
	, m_vm(vm) {}

auto Canvas::clear() noexcept -> void {
	m_modified = true;
	m_items.clear();
	m_menuStack.clear();
}

auto Canvas::isClear() const noexcept -> bool {
	return m_items.empty();
}

auto Canvas::addButton(Id id, Vec2 position, Vec2 size, Color color, std::string text, std::shared_ptr<Environment> env,
					   std::shared_ptr<Process> process, std::string_view command) -> bool {
	return this->addItem<Button>(
		id,
		std::make_unique<CmdButton>(position, size, color, std::move(text), m_game, m_vm, std::move(env), std::move(process), command));
}

auto Canvas::addInput(Id id, Vec2 position, Vec2 size, Color color, std::string text, std::shared_ptr<Environment> env,
					  std::shared_ptr<Process> process, std::string_view command, std::size_t maxLength, bool isPrivate, bool replaceMode) -> bool {
	return this->addItem<Input>(
		id,
		std::make_unique<CmdInput>(position, size, color, std::move(text), m_game, m_vm, std::move(env), std::move(process), command, maxLength, isPrivate, replaceMode));
}

auto Canvas::addSlider(Id id, Vec2 position, Vec2 size, Color color, float value, float delta, std::shared_ptr<Environment> env,
					   std::shared_ptr<Process> process, std::string_view command) -> bool {
	return this->addItem<Slider>(id, std::make_unique<CmdSlider>(position, size, color, value, delta, m_game, m_vm, std::move(env), std::move(process), command));
}

auto Canvas::addCheckbox(Id id, Vec2 position, Vec2 size, Color color, bool value, std::shared_ptr<Environment> env,
						 std::shared_ptr<Process> process, std::string_view command) -> bool {
	return this->addItem<Checkbox>(id, std::make_unique<CmdCheckbox>(position, size, color, value, m_game, m_vm, std::move(env), std::move(process), command));
}

auto Canvas::addDropdown(Id id, Vec2 position, Vec2 size, Color color, std::vector<std::string> options, std::size_t selectedOptionIndex,
						 std::shared_ptr<Environment> env, std::shared_ptr<Process> process, std::string_view command) -> bool {
	return this->addItem<Dropdown>(
		id,
		std::make_unique<CmdDropdown>(position, size, color, std::move(options), selectedOptionIndex, m_game, m_vm, std::move(env), std::move(process), command));
}

auto Canvas::addScreen(Id id, Vec2 position, Color color, util::TileMatrix<char> screen) -> bool {
	return this->addItem<Screen>(id, position, color, std::move(screen));
}

auto Canvas::addText(Id id, Vec2 position, Color color, std::string text) -> bool {
	return this->addItem<Text>(id, position, color, std::move(text));
}

auto Canvas::pushMenu(util::Span<const Id> ids, const std::shared_ptr<Environment>& env, const std::shared_ptr<Process>& process,
					  std::string_view selectNoneCommand, std::string_view escapeCommand, std::string_view directionCommand,
					  std::string_view clickCommand, std::string_view scrollCommand, std::string_view hoverCommand) -> bool {
	if (ids.empty()) {
		return false;
	}

	auto elements = std::vector<Element*>{};
	for (const auto& id : ids) {
		if (const auto it = this->findItem(id); it != m_items.end()) {
			if (!util::match(it->item)(
					[&](auto& elem) {
						elements.push_back(elem.get());
						return true;
					},
					[](Screen&) { return false; },
					[](Text&) { return false; })) {
				return false;
			}
		} else {
			return false;
		}
	}

	this->deactivate();

	m_modified = true;

	auto onSelectNone = gui::Menu::Function{nullptr};
	if (!selectNoneCommand.empty()) {
		onSelectNone = gui::Menu::Function{[&game = m_game, &vm = m_vm, env, process, script = Script::parse(selectNoneCommand)](gui::Menu&) {
			if (const auto frame = process->call(std::make_shared<Environment>(env), script)) {
				vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
			}
		}};
	}

	auto onEscape = gui::Menu::Function{nullptr};
	if (!escapeCommand.empty()) {
		onEscape = gui::Menu::Function{[&game = m_game, &vm = m_vm, env, process, script = Script::parse(escapeCommand)](gui::Menu&) {
			if (const auto frame = process->call(std::make_shared<Environment>(env), script)) {
				vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
			}
		}};
	}

	auto onDirection = gui::Menu::DirectionFunction{nullptr};
	if (!directionCommand.empty()) {
		onDirection = [&game = m_game, &vm = m_vm, env, process, command = std::string{directionCommand}](gui::Menu&, Vec2 offset) {
			const auto diffX = util::toString(offset.x);
			const auto diffY = util::toString(offset.y);
			if (const auto frame = process->call(std::make_shared<Environment>(env), Script::command({command, diffX, diffY}))) {
				vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
			}
		};
	}

	auto onClick = gui::Menu::ClickFunction{nullptr};
	if (!clickCommand.empty()) {
		onClick = [&game = m_game, &vm = m_vm, env, process, command = std::string{clickCommand}](gui::Menu&, Vec2 position) {
			const auto x = util::toString(position.x);
			const auto y = util::toString(position.y);
			if (const auto frame = process->call(std::make_shared<Environment>(env), Script::command({command, x, y}))) {
				vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
			}
		};
	}

	auto onScroll = gui::Menu::ScrollFunction{nullptr};
	if (!scrollCommand.empty()) {
		onScroll = [&game = m_game, &vm = m_vm, env, process, command = std::string{scrollCommand}](gui::Menu&, Vec2 position, Vec2 offset) {
			const auto x = util::toString(position.x);
			const auto y = util::toString(position.y);
			const auto diffX = util::toString(offset.x);
			const auto diffY = util::toString(offset.y);
			if (const auto frame = process->call(std::make_shared<Environment>(env), Script::command({command, x, y, diffX, diffY}))) {
				vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
			}
		};
	}

	auto onHover = gui::Menu::HoverFunction{nullptr};
	if (!hoverCommand.empty()) {
		onHover = [&game = m_game, &vm = m_vm, env, process, command = std::string{hoverCommand}](gui::Menu&, Vec2 position) {
			const auto x = util::toString(position.x);
			const auto y = util::toString(position.y);
			if (const auto frame = process->call(std::make_shared<Environment>(env), Script::command({command, x, y}))) {
				vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
			}
		};
	}

	m_menuStack.emplace_back(std::move(elements),
							 std::move(onSelectNone),
							 std::move(onEscape),
							 std::move(onDirection),
							 std::move(onClick),
							 std::move(onScroll),
							 std::move(onHover));
	return true;
}

auto Canvas::hasMenu() const noexcept -> bool {
	return !m_menuStack.empty();
}

auto Canvas::menuStackSize() const noexcept -> std::size_t {
	return m_menuStack.size();
}

auto Canvas::popMenu() noexcept -> bool {
	if (m_menuStack.empty()) {
		return false;
	}

	m_modified = true;
	m_menuStack.pop_back();
	return true;
}

auto Canvas::hasElement(Id id) const -> bool {
	return util::anyOf(m_items, [id](const auto& item) { return item.id == id; });
}

auto Canvas::removeElement(Id id) -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		m_modified = true;
		util::match(it->item)(
			[&](auto& elem) {
				for (auto& menu : m_menuStack) {
					menu.removeElement(*elem);
				}
			},
			[](Screen&) {},
			[](Text&) {});
		m_items.erase(it);
		return true;
	}
	return false;
}

auto Canvas::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	m_modified = false;
	if (!m_menuStack.empty()) {
		m_menuStack.back().handleEvent(e, charWindow);
		if (!m_modified) {
			for (auto&& element : m_menuStack.back().elements()) {
				element->handleEvent(e, charWindow);
				if (m_modified) {
					break;
				}
			}
		}
	} else {
		for (auto& item : m_items) {
			util::match(item.item)( //
				[&](auto& elem) { elem->handleEvent(e, charWindow); },
				[](Screen&) {},
				[](Text&) {});
			if (m_modified) {
				break;
			}
		}
	}
}

auto Canvas::update(float deltaTime) -> void {
	m_modified = false;
	if (!m_menuStack.empty()) {
		for (auto&& element : m_menuStack.back().elements()) {
			element->update(deltaTime);
			if (m_modified) {
				break;
			}
		}
	} else {
		for (auto& item : m_items) {
			util::match(item.item)([&](auto& elem) { elem->update(deltaTime); }, [](Screen&) {}, [](Text&) {});
			if (m_modified) {
				break;
			}
		}
	}
}

auto Canvas::draw(CharWindow& charWindow) -> void {
	for (const auto& item : m_items) {
		util::match(item.item)([&](const auto& elem) { elem->draw(charWindow); },
							   [&](const Screen& screen) { charWindow.draw(screen.position, screen.screen, screen.color); },
							   [&](const Text& text) { charWindow.draw(text.position, text.text, text.color); });
	}
}

auto Canvas::isElementActivated(Id id) const -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)([](const auto& elem) { return elem->isActivated(); },
									 [](const Screen&) { return false; },
									 [](const Text&) { return false; });
	}
	return false;
}

auto Canvas::activateElement(Id id) -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)(
			[&](auto& elem) {
				if (!m_menuStack.empty()) {
					m_menuStack.back().activateElement(*elem);
				} else {
					elem->activate();
				}
				return true;
			},
			[](Screen&) { return false; },
			[](Text&) { return false; });
	}
	return false;
}

auto Canvas::deactivateElement(Id id) -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)(
			[&](auto& elem) {
				if (!m_menuStack.empty() && m_menuStack.back().getActiveElement() == elem.get()) {
					m_menuStack.back().deactivate();
				}
				elem->deactivate();
				return true;
			},
			[](Screen&) { return false; },
			[](Text&) { return false; });
	}
	return false;
}

auto Canvas::deactivate() -> void {
	m_modified = false;
	if (!m_menuStack.empty()) {
		m_menuStack.back().deactivate();
	}

	if (!m_modified) {
		for (auto& item : m_items) {
			util::match(item.item)([](auto& elem) { elem->deactivate(); }, [](Screen&) {}, [](Text&) {});
			if (m_modified) {
				break;
			}
		}
	}
}

auto Canvas::getElementText(Id id) const -> std::optional<std::string> {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)([](const Button& button) -> std::optional<std::string> { return std::string{button->getText()}; },
									 [](const Input& input) -> std::optional<std::string> { return std::string{input->getText()}; },
									 [](const Slider&) -> std::optional<std::string> { return std::nullopt; },
									 [](const Checkbox&) -> std::optional<std::string> { return std::nullopt; },
									 [](const Dropdown& dropdown) -> std::optional<std::string> {
										 auto result = std::string{};
										 if (dropdown->getOptionCount() > 0) {
											 result = dropdown->getOption(0);
											 for (auto i = std::size_t{1}; i < dropdown->getOptionCount(); ++i) {
												 result.append(fmt::format("\n{}", Script::escapedString(dropdown->getOption(i))));
											 }
										 }
										 return result;
									 },
									 [](const Screen&) -> std::optional<std::string> { return std::nullopt; },
									 [](const Text& text) -> std::optional<std::string> { return text.text; });
	}
	return std::nullopt;
}

auto Canvas::getElementColor(Id id) const -> std::optional<Color> {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)([](const auto& elem) -> std::optional<Color> { return elem->getColor(); },
									 [](const Screen& screen) -> std::optional<Color> { return screen.color; },
									 [](const Text& text) -> std::optional<Color> { return text.color; });
	}
	return std::nullopt;
}

auto Canvas::getElementValue(Id id) const -> std::optional<float> {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)(
			[](const Button& button) -> std::optional<float> { return (button->getState() == gui::Button::State::PRESSED) ? 1.0f : 0.0f; },
			[](const Input&) -> std::optional<float> { return std::nullopt; },
			[](const Slider& slider) -> std::optional<float> { return slider->getValue(); },
			[](const Checkbox& checkbox) -> std::optional<float> { return (checkbox->getValue()) ? 1.0f : 0.0f; },
			[](const Dropdown& dropdown) -> std::optional<float> { return static_cast<float>(dropdown->getSelectedOptionIndex()); },
			[](const Screen&) -> std::optional<float> { return std::nullopt; },
			[](const Text&) -> std::optional<float> { return std::nullopt; });
	}
	return std::nullopt;
}

auto Canvas::setElementText(Id id, std::string str) -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)(
			[&](Button& button) {
				button->setText(std::move(str));
				return true;
			},
			[&](Input& input) {
				input->setText(std::move(str));
				return true;
			},
			[](Slider&) { return false; },
			[](Checkbox&) { return false; },
			[](Dropdown&) { return false; },
			[](Screen&) { return false; },
			[&](Text& text) {
				text.text = std::move(str);
				return true;
			});
	}
	return false;
}

auto Canvas::setElementColor(Id id, Color color) -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)(
			[&](auto& elem) {
				elem->setColor(color);
				return true;
			},
			[&](Screen& screen) {
				screen.color = color;
				return true;
			},
			[&](Text& text) {
				text.color = color;
				return true;
			});
	}
	return false;
}

auto Canvas::setElementValue(Id id, float value) -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)([](Button&) { return false; },
									 [](Input&) { return false; },
									 [&](Slider& slider) {
										 slider->setValue(value);
										 return true;
									 },
									 [&](Checkbox& checkbox) {
										 checkbox->setValue(value != 0.0f);
										 return true;
									 },
									 [&](Dropdown& dropdown) {
										 dropdown->setSelectedOptionIndex(static_cast<std::size_t>(value));
										 return true;
									 },
									 [](Screen&) { return false; },
									 [](Text&) { return false; });
	}
	return false;
}

auto Canvas::getScreenChar(Id id, std::size_t x, std::size_t y, char defaultVal) const -> std::optional<char> {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)([](const Button&) -> std::optional<char> { return std::nullopt; },
									 [](const Input&) -> std::optional<char> { return std::nullopt; },
									 [](const Slider&) -> std::optional<char> { return std::nullopt; },
									 [](const Checkbox&) -> std::optional<char> { return std::nullopt; },
									 [](const Dropdown&) -> std::optional<char> { return std::nullopt; },
									 [&](const Screen& screen) -> std::optional<char> { return screen.screen.get(x, y, defaultVal); },
									 [](const Text&) -> std::optional<char> { return std::nullopt; });
	}
	return std::nullopt;
}

auto Canvas::setScreenChar(Id id, std::size_t x, std::size_t y, char ch) -> bool {
	if (const auto it = this->findItem(id); it != m_items.end()) {
		return util::match(it->item)([](Button&) { return false; },
									 [](Input&) { return false; },
									 [](Slider&) { return false; },
									 [](Checkbox&) { return false; },
									 [](Dropdown&) { return false; },
									 [&](Screen& screen) {
										 screen.screen.set(x, y, ch);
										 return true;
									 },
									 [](Text&) { return false; });
	}
	return false;
}

auto Canvas::getElementInfo() const -> std::vector<Canvas::ElementInfoView> {
	static constexpr auto getInfoView = [](const auto& item) {
		return util::match(item.item)(
			[&](const Button& button) -> ElementInfoView {
				return ButtonInfoView{item.id, button->getPosition(), button->getSize(), button->getColor(), button->getText(), button->isActivated()};
			},
			[&](const Input& input) -> ElementInfoView {
				return InputInfoView{item.id, input->getPosition(), input->getSize(), input->getColor(), input->getText(), input->isActivated()};
			},
			[&](const Slider& slider) -> ElementInfoView {
				return SliderInfoView{item.id, slider->getPosition(), slider->getSize(), slider->getColor(), slider->isActivated()};
			},
			[&](const Checkbox& checkbox) -> ElementInfoView {
				return CheckboxInfoView{item.id, checkbox->getPosition(), checkbox->getSize(), checkbox->getColor(), checkbox->isActivated()};
			},
			[&](const Dropdown& dropdown) -> ElementInfoView {
				return DropdownInfoView{item.id, dropdown->getPosition(), dropdown->getSize(), dropdown->getColor(), dropdown->isActivated()};
			},
			[&](const Screen& screen) -> ElementInfoView {
				return ScreenInfoView{item.id,
									  screen.position,
									  Vec2{static_cast<Vec2::Length>(screen.screen.getWidth()), static_cast<Vec2::Length>(screen.screen.getHeight())},
									  screen.color};
			},
			[&](const Text& text) -> ElementInfoView {
				return TextInfoView{item.id, text.position, text.color, text.text};
			});
	};

	return m_items | util::transform(getInfoView) | util::collect<std::vector<ElementInfoView>>();
}

auto Canvas::getMenuInfo() const -> std::vector<Canvas::MenuInfoView> {
	const auto getInfoView = [&](const auto& menu) {
		const auto* const activeElement = menu.getActiveElement();
		auto menuInfoView = MenuInfoView{std::vector<Id>{}, &menu == &m_menuStack.back(), (activeElement) ? this->findId(*activeElement) : std::nullopt};
		for (const auto& element : menu.elements()) {
			if (const auto id = this->findId(*element)) {
				menuInfoView.ids.push_back(*id);
			}
		}
		return menuInfoView;
	};

	return m_menuStack | util::transform(getInfoView) | util::collect<std::vector<MenuInfoView>>();
}

auto Canvas::getElementIds() const -> std::vector<Canvas::Id> {
	static constexpr auto getId = [](const auto& item) {
		return item.id;
	};
	return m_items | util::transform(getId) | util::collect<std::vector<Id>>();
}

auto Canvas::findItem(Id id) -> std::vector<Canvas::Item>::iterator {
	return util::findIf(m_items, [id](const auto& item) { return item.id == id; });
}

auto Canvas::findItem(Id id) const -> std::vector<Canvas::Item>::const_iterator {
	return util::findIf(m_items, [id](const auto& item) { return item.id == id; });
}

auto Canvas::findId(const Element& element) const -> std::optional<Canvas::Id> {
	if (const auto it = util::findIf(m_items,
									 [&](const auto& item) {
										 return util::match(item.item)([&](const auto& elem) { return elem.get() == &element; },
																	   [](const Screen&) { return false; },
																	   [](const Text&) { return false; });
									 });
		it != m_items.end()) {
		return it->id;
	}
	return std::nullopt;
}

} // namespace gui
