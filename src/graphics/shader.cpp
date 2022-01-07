#include "shader.hpp"

#include "error.hpp"  // gfx::Error
#include "opengl.hpp" // GL..., gl...

#include <fstream>     // std::ifstream
#include <sstream>     // std::ostringstream
#include <string_view> // std::string_view

namespace gfx {

Shader::Shader(ShaderType type, const char* filepath)
	: m_shader((filepath) ? Shader::makeShaderObject(type) : ShaderObject{}) {
	if (!m_shader) {
		return;
	}

	auto file = std::ifstream{filepath};
	if (!file) {
		throw Error{fmt::format("Failed to read shader code file \"{}\"!\n", filepath)};
	}
	auto stream = std::ostringstream{};
	stream << file.rdbuf();
	const auto source = std::move(stream).str();
	file.close();

#ifdef __EMSCRIPTEN__
	constexpr auto header = std::string_view{"#version 300 es\nprecision highp float;\n"};
#else
	constexpr auto header = std::string_view{"#version 330 core\n"};
#endif

	const auto sourceStrings = std::array{static_cast<const GLchar*>(header.data()), static_cast<const GLchar*>(source.data())};
	const auto sourceLengths = std::array{static_cast<GLint>(header.size()), static_cast<GLint>(source.size())};
	glShaderSource(m_shader.get(), sourceStrings.size(), sourceStrings.data(), sourceLengths.data());
	glCompileShader(m_shader.get());

	auto success = GLint{GL_FALSE};
	glGetShaderiv(m_shader.get(), GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		auto infoLogLength = GLint{0};
		glGetShaderiv(m_shader.get(), GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0) {
			auto infoLog = std::string(static_cast<std::size_t>(infoLogLength), '\0');
			glGetShaderInfoLog(m_shader.get(), infoLogLength, nullptr, infoLog.data());
			throw Error{fmt::format("Failed to compile shader \"{}\":\n{}", filepath, infoLog.c_str())};
		}
		throw Error{fmt::format("Failed to compile shader \"{}\"!", filepath)};
	}
}

Shader::operator bool() const noexcept {
	return static_cast<bool>(m_shader);
}

auto Shader::get() const noexcept -> Handle {
	return m_shader.get();
}

auto Shader::makeShaderObject(ShaderType type) -> ShaderObject {
	const auto handle = glCreateShader(static_cast<GLenum>(type));
	if (!handle) {
		throw Error{"Failed to create shader object!"};
	}
	return ShaderObject{handle};
}

auto Shader::ShaderDeleter::operator()(Handle handle) const noexcept -> void {
	glDeleteShader(handle);
}

ShaderProgram::ShaderProgram(const char* vertexShaderFilepath, const char* fragmentShaderFilepath)
	: m_program((vertexShaderFilepath || fragmentShaderFilepath) ? ShaderProgram::makeProgramObject() : ProgramObject{})
	, m_vertexShader(ShaderType::VERTEX_SHADER, vertexShaderFilepath)
	, m_fragmentShader(ShaderType::FRAGMENT_SHADER, fragmentShaderFilepath) {
	if (!m_program) {
		return;
	}

	if (m_vertexShader) {
		glAttachShader(m_program.get(), m_vertexShader.get());
	}
	if (m_fragmentShader) {
		glAttachShader(m_program.get(), m_fragmentShader.get());
	}

	glLinkProgram(m_program.get());

	auto success = GLint{GL_FALSE};
	glGetProgramiv(m_program.get(), GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		auto infoLogLength = GLint{0};
		glGetProgramiv(m_program.get(), GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0) {
			auto infoLog = std::string(static_cast<std::size_t>(infoLogLength), '\0');
			glGetProgramInfoLog(m_program.get(), infoLogLength, nullptr, infoLog.data());
			throw Error{fmt::format("Failed to link shader program:\n{}", infoLog.c_str())};
		}
		throw Error{"Failed to link shader program!"};
	}
}

ShaderProgram::operator bool() const noexcept {
	return static_cast<bool>(m_program);
}

auto ShaderProgram::get() const noexcept -> Handle {
	return m_program.get();
}

auto ShaderProgram::makeProgramObject() -> ProgramObject {
	const auto handle = glCreateProgram();
	if (!handle) {
		throw Error{"Failed to create shader program object!"};
	}
	return ProgramObject{handle};
}

auto ShaderProgram::ProgramDeleter::operator()(Handle handle) const noexcept -> void {
	glDeleteProgram(handle);
}

ShaderUniform::ShaderUniform(const ShaderProgram& program, const char* name) noexcept
	: m_location((program) ? glGetUniformLocation(program.get(), name) : -1) {}

auto ShaderUniform::getLocation() const noexcept -> std::int32_t {
	return m_location;
}

} // namespace gfx
