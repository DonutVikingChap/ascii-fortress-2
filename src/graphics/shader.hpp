#ifndef AF2_GRAPHICS_SHADER_HPP
#define AF2_GRAPHICS_SHADER_HPP

#include "../utilities/resource.hpp" // util::Resource
#include "handle.hpp"                // gfx::Handle

#include <array>      // std::array
#include <cstddef>    // std::size_t
#include <cstdint>    // std::int32_t
#include <fmt/core.h> // fmt::format
#include <utility>    // std::move, std::index_sequence, std::make_index_sequence

namespace gfx {

enum class ShaderType : unsigned {
	VERTEX_SHADER = 0x8B31,
	FRAGMENT_SHADER = 0x8B30,
};

class Shader final {
public:
	explicit Shader(ShaderType type, const char* filepath = nullptr);

	explicit operator bool() const noexcept;

	[[nodiscard]] auto get() const noexcept -> Handle;

private:
	struct ShaderDeleter final {
		auto operator()(Handle handle) const noexcept -> void;
	};
	using ShaderObject = util::Resource<Handle, ShaderDeleter>;

	[[nodiscard]] static auto makeShaderObject(ShaderType type) -> ShaderObject;

	ShaderObject m_shader;
};

class ShaderProgram final {
public:
	explicit ShaderProgram(const char* vertexShaderFilepath = nullptr, const char* fragmentShaderFilepath = nullptr);

	explicit operator bool() const noexcept;

	[[nodiscard]] auto get() const noexcept -> Handle;

private:
	struct ProgramDeleter final {
		auto operator()(Handle handle) const noexcept -> void;
	};
	using ProgramObject = util::Resource<Handle, ProgramDeleter>;

	[[nodiscard]] static auto makeProgramObject() -> ProgramObject;

	ProgramObject m_program;
	Shader m_vertexShader;
	Shader m_fragmentShader;
};

class ShaderUniform final {
public:
	ShaderUniform(const ShaderProgram& program, const char* name) noexcept;

	[[nodiscard]] auto getLocation() const noexcept -> std::int32_t;

private:
	std::int32_t m_location;
};

template <typename T, std::size_t N>
class ShaderArray final {
public:
	ShaderArray(const ShaderProgram& program, const char* name)
		: m_arr(ShaderArray::createArray(program, name, std::make_index_sequence<N>{})) {}

	[[nodiscard]] auto size() const noexcept -> std::size_t {
		return m_arr.size();
	}

	[[nodiscard]] auto operator[](std::size_t i) -> T& {
		return m_arr[i];
	}

	[[nodiscard]] auto operator[](std::size_t i) const -> const T& {
		return m_arr[i];
	}

	[[nodiscard]] auto begin() const noexcept -> decltype(auto) {
		return m_arr.begin();
	}

	[[nodiscard]] auto end() const noexcept -> decltype(auto) {
		return m_arr.end();
	}

private:
	template <std::size_t... INDICES>
	[[nodiscard]] static auto createArray(const ShaderProgram& program, const char* name, std::index_sequence<INDICES...>) -> std::array<T, N> {
		return std::array<T, N>{(T{program, fmt::format("{}[{}]", name, INDICES).c_str()})...};
	}

	std::array<T, N> m_arr;
};

} // namespace gfx

#endif
