#include "call_frame_handle.hpp"

#include "process.hpp" // Process

#include <cassert> // assert

CallFrameHandle::CallFrameHandle(std::shared_ptr<Process> process, std::size_t frameIndex) noexcept
	: m_process(std::move(process))
	, m_frameIndex(frameIndex) {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
}

auto CallFrameHandle::process() const noexcept -> const std::shared_ptr<Process>& {
	return m_process;
}

auto CallFrameHandle::index() const noexcept -> std::size_t {
	return m_frameIndex;
}

auto CallFrameHandle::env() const noexcept -> const std::shared_ptr<Environment>& {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->m_callStack[m_frameIndex].env;
}

auto CallFrameHandle::retFrame() const noexcept -> std::size_t {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->m_callStack[m_frameIndex].returnFrameIndex;
}

auto CallFrameHandle::retArg() const noexcept -> std::size_t {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->m_callStack[m_frameIndex].returnArgumentIndex;
}

auto CallFrameHandle::status() const noexcept -> cmd::Status {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->m_callStack[m_frameIndex].status;
}

auto CallFrameHandle::progress() const noexcept -> cmd::Progress {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	auto& frame = m_process->m_callStack[m_frameIndex];
	assert(frame.programCounter < frame.commandStates.size());
	return frame.commandStates[frame.programCounter].progress;
}

auto CallFrameHandle::arguments() const noexcept -> cmd::CommandArguments& {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	auto& frame = m_process->m_callStack[m_frameIndex];
	assert(frame.programCounter < frame.commandStates.size());
	return frame.commandStates[frame.programCounter].arguments;
}

auto CallFrameHandle::data() const noexcept -> std::any& {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	auto& frame = m_process->m_callStack[m_frameIndex];
	assert(frame.programCounter < frame.commandStates.size());
	return frame.commandStates[frame.programCounter].data;
}

auto CallFrameHandle::getExportTarget() const noexcept -> std::shared_ptr<Environment> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->m_callStack[m_frameIndex].exportTarget.lock();
}

auto CallFrameHandle::makeTryBlock() const noexcept -> void {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	m_process->m_callStack[m_frameIndex].firstInTryBlock = true;
}

auto CallFrameHandle::makeSection() const noexcept -> void {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	m_process->m_callStack[m_frameIndex].firstInSection = true;
}

auto CallFrameHandle::resetExportTarget() const noexcept -> void {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	m_process->m_callStack[m_frameIndex].exportTarget.reset();
}

auto CallFrameHandle::setExportTarget(const std::shared_ptr<Environment>& exportTarget) const noexcept -> void {
	assert(exportTarget);
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	m_process->m_callStack[m_frameIndex].exportTarget = exportTarget;
}

auto CallFrameHandle::run(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) const -> cmd::Result {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->run(game, server, client, metaServer, metaClient, this->index());
}

auto CallFrameHandle::await(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) const -> cmd::Result {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->await(game, server, client, metaServer, metaClient, this->index());
}

auto CallFrameHandle::awaitUnlimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) const
	-> cmd::Result {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->awaitUnlimited(game, server, client, metaServer, metaClient, this->index());
}

auto CallFrameHandle::awaitLimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient,
								   int limit) const -> cmd::Result {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->awaitLimited(game, server, client, metaServer, metaClient, limit, this->index());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, std::string_view script) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), script, this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, cmd::CommandView argv) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), argv, this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, Script::Command command) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), std::move(command), this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, Script commands) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), std::move(commands), this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, const Environment::Function& function) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), function, this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, const Environment::Function& function,
						   util::Span<const cmd::Value> args) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), function, args, this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConCommand& cmd) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cmd, this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConCommand& cmd,
						   util::Span<const cmd::Value> args) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cmd, args, this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConVar& cvar) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cvar, this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConVar& cvar, std::string value) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cvar, std::move(value), this->index(), returnArgumentIndex, this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, std::string_view script) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), script, this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, cmd::CommandView argv) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), argv, this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, Script::Command command) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), std::move(command), this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, Script commands) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), std::move(commands), this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, const Environment::Function& function) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), function, this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, const Environment::Function& function, util::Span<const cmd::Value> args) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), function, args, this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, ConCommand& cmd) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cmd, this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, ConCommand& cmd, util::Span<const cmd::Value> args) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cmd, args, this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, ConVar& cvar) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cvar, this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::tailCall(std::shared_ptr<Environment> env, ConVar& cvar, std::string value) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cvar, std::move(value), this->retFrame(), this->retArg(), this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, std::string_view script) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), script, Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, cmd::CommandView argv) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), argv, Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, Script::Command command) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), std::move(command), Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, Script commands) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), std::move(commands), Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, const Environment::Function& function) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), function, Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, const Environment::Function& function, util::Span<const cmd::Value> args) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), function, args, Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, ConCommand& cmd) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cmd, Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, ConCommand& cmd, util::Span<const cmd::Value> args) const
	-> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cmd, args, Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, ConVar& cvar) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cvar, Process::NO_FRAME, 0, this->getExportTarget());
}

auto CallFrameHandle::callDiscard(std::shared_ptr<Environment> env, ConVar& cvar, std::string value) const -> std::optional<CallFrameHandle> {
	assert(m_process);
	assert(m_frameIndex < m_process->m_callStack.size());
	return m_process->call(std::move(env), cvar, std::move(value), Process::NO_FRAME, 0, this->getExportTarget());
}
