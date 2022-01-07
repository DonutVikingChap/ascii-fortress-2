#ifndef AF2_NETWORK_CRYPTO_HPP
#define AF2_NETWORK_CRYPTO_HPP

#include "../utilities/algorithm.hpp" // util::copy
#include "../utilities/crc.hpp"       // util::CRC32
#include "../utilities/span.hpp"      // util::Span

#include <algorithm>   // std::find, std::equal
#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t, std::byte
#include <cstdint>     // std::uint8_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <sodium.h>    // sodium..., crypto_...
#include <string>      // std::string
#include <string_view> // std::string_view

namespace crypto {

namespace detail {

template <typename T, typename CharType, std::size_t N_BYTES>
class ByteArray {
public:
	static constexpr auto SIZE = N_BYTES;

private:
	using Bytes = std::array<CharType, SIZE>;

public:
	using size_type = typename Bytes::size_type;
	using difference_type = typename Bytes::difference_type;
	using value_type = typename Bytes::value_type;
	using reference = typename Bytes::reference;
	using const_reference = typename Bytes::const_reference;
	using pointer = typename Bytes::pointer;
	using const_pointer = typename Bytes::const_pointer;
	using iterator = typename Bytes::iterator;
	using const_iterator = typename Bytes::const_iterator;
	using reverse_iterator = typename Bytes::reverse_iterator;
	using const_reverse_iterator = typename Bytes::const_reverse_iterator;

	[[nodiscard]] constexpr auto at(size_type i) -> reference {
		return m_bytes.at(i);
	}

	[[nodiscard]] constexpr auto at(size_type i) const -> const_reference {
		return m_bytes.at(i);
	}

	[[nodiscard]] constexpr auto operator[](size_type i) -> reference {
		return m_bytes[i];
	}

	[[nodiscard]] constexpr auto operator[](size_type i) const -> const_reference {
		return m_bytes[i];
	}

	[[nodiscard]] constexpr auto front() -> reference {
		return m_bytes.front();
	}

	[[nodiscard]] constexpr auto front() const -> const_reference {
		return m_bytes.front();
	}

	[[nodiscard]] constexpr auto back() -> reference {
		return m_bytes.back();
	}

	[[nodiscard]] constexpr auto back() const -> const_reference {
		return m_bytes.back();
	}

	[[nodiscard]] constexpr auto data() noexcept -> pointer {
		return m_bytes.data();
	}

	[[nodiscard]] constexpr auto data() const noexcept -> const_pointer {
		return m_bytes.data();
	}

	[[nodiscard]] constexpr auto begin() noexcept -> iterator {
		return m_bytes.begin();
	}

	[[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {
		return m_bytes.begin();
	}

	[[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator {
		return m_bytes.cbegin();
	}

	[[nodiscard]] constexpr auto end() noexcept -> iterator {
		return m_bytes.end();
	}

	[[nodiscard]] constexpr auto end() const noexcept -> const_iterator {
		return m_bytes.end();
	}

	[[nodiscard]] constexpr auto cend() const noexcept -> const_iterator {
		return m_bytes.cend();
	}

	[[nodiscard]] constexpr auto rbegin() noexcept -> reverse_iterator {
		return m_bytes.rbegin();
	}

	[[nodiscard]] constexpr auto rbegin() const noexcept -> const_reverse_iterator {
		return m_bytes.rbegin();
	}

	[[nodiscard]] constexpr auto crbegin() const noexcept -> const_reverse_iterator {
		return m_bytes.crbegin();
	}

	[[nodiscard]] constexpr auto rend() noexcept -> reverse_iterator {
		return m_bytes.rend();
	}

	[[nodiscard]] constexpr auto rend() const noexcept -> const_reverse_iterator {
		return m_bytes.rend();
	}

	[[nodiscard]] constexpr auto crend() const noexcept -> const_reverse_iterator {
		return m_bytes.crend();
	}

	[[nodiscard]] constexpr auto empty() const noexcept -> bool {
		return m_bytes.empty();
	}

	[[nodiscard]] constexpr auto size() const noexcept -> size_type {
		return m_bytes.size();
	}

	[[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
		return m_bytes.max_size();
	}

	auto fill(const value_type& value) noexcept -> void {
		m_bytes.fill(value);
	}

	auto swap(T& other) noexcept -> void {
		m_bytes.swap(other.m_bytes);
	}

	[[nodiscard]] friend auto operator==(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_bytes == rhs.m_bytes;
	}

	[[nodiscard]] friend auto operator!=(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_bytes != rhs.m_bytes;
	}

	[[nodiscard]] friend auto operator<(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_bytes < rhs.m_bytes;
	}

	[[nodiscard]] friend auto operator<=(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_bytes <= rhs.m_bytes;
	}

	[[nodiscard]] friend auto operator>(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_bytes > rhs.m_bytes;
	}

	[[nodiscard]] friend auto operator>=(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_bytes >= rhs.m_bytes;
	}

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const T& val) -> Stream& {
		return stream << val.m_bytes;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, T& val) -> Stream& {
		return stream >> val.m_bytes;
	}

private:
	Bytes m_bytes;
};

} // namespace detail

[[nodiscard]] inline auto init() noexcept -> bool {
	return sodium_init() >= 0;
}

class Seed final : public crypto::detail::ByteArray<Seed, unsigned char, randombytes_SEEDBYTES> {};
using SeedRef = util::Span<Seed::value_type, Seed::SIZE>;
using SeedView = util::Span<const Seed::value_type, Seed::SIZE>;

class AccessToken final : public crypto::detail::ByteArray<AccessToken, unsigned char, 32> {};
using AccessTokenRef = util::Span<AccessToken::value_type, AccessToken::SIZE>;
using AccessTokenView = util::Span<const AccessToken::value_type, AccessToken::SIZE>;

class FastHash final : public crypto::detail::ByteArray<FastHash, unsigned char, crypto_generichash_blake2b_BYTES> {};
using FastHashRef = util::Span<FastHash::value_type, FastHash::SIZE>;
using FastHashView = util::Span<const FastHash::value_type, FastHash::SIZE>;

inline auto generateSeed(SeedRef result) noexcept -> void {
	randombytes_buf(result.data(), result.size());
}

inline auto generateRandomData(util::Span<std::byte> result) noexcept -> void {
	randombytes_buf(result.data(), result.size());
}

inline auto generateRandomData(util::Span<std::byte> result, SeedView seed) noexcept -> void {
	randombytes_buf_deterministic(result.data(), result.size(), seed.data());
}

inline auto generateAccessToken(AccessTokenRef result) noexcept -> void {
	randombytes_buf(result.data(), result.size());
}

inline auto generateAccessToken(AccessTokenRef result, SeedView seed) noexcept -> void {
	randombytes_buf_deterministic(result.data(), result.size(), seed.data());
}

[[nodiscard]] inline auto verifyAccessToken(AccessTokenView token, AccessTokenView input) noexcept -> bool {
	return std::equal(token.begin(), token.end(), input.begin());
}

[[nodiscard]] inline auto fastHash(FastHashRef result, util::Span<const std::byte> data) noexcept -> bool {
	return crypto_generichash_blake2b(result.data(), result.size(), reinterpret_cast<const unsigned char*>(data.data()), data.size(), nullptr, 0) == 0;
}

[[nodiscard]] inline auto verifyFastHash(FastHashView hash, util::Span<const std::byte> data) noexcept -> bool {
	auto dataHash = FastHash{};
	if (!crypto::fastHash(dataHash, data)) {
		return false;
	}
	return std::equal(dataHash.begin(), dataHash.end(), hash.begin());
}

// Password library.
namespace pw {

enum class HashType : std::uint8_t { FAST, MEDIUM, SLOW };

[[nodiscard]] constexpr auto getHashTypeString(HashType type) noexcept -> std::string_view {
	switch (type) {
		case HashType::SLOW: return "slow";
		case HashType::MEDIUM: return "medium";
		case HashType::FAST: return "fast";
	}
	return std::string_view{};
}

[[nodiscard]] constexpr auto getHashType(std::string_view str) noexcept -> std::optional<HashType> {
	if (str == getHashTypeString(HashType::SLOW)) {
		return HashType::SLOW;
	}
	if (str == getHashTypeString(HashType::MEDIUM)) {
		return HashType::MEDIUM;
	}
	if (str == getHashTypeString(HashType::FAST)) {
		return HashType::FAST;
	}
	return std::nullopt;
}

namespace detail {

[[nodiscard]] constexpr auto getOpsLimit(HashType type) noexcept -> int {
	switch (type) {
		case HashType::FAST: return crypto_pwhash_OPSLIMIT_INTERACTIVE;
		case HashType::MEDIUM: return crypto_pwhash_OPSLIMIT_MODERATE;
		case HashType::SLOW: return crypto_pwhash_OPSLIMIT_SENSITIVE;
	}
	assert(false);
	return -1;
}

[[nodiscard]] constexpr auto getMemLimit(HashType type) noexcept -> int {
	switch (type) {
		case HashType::FAST: return crypto_pwhash_MEMLIMIT_INTERACTIVE;
		case HashType::MEDIUM: return crypto_pwhash_MEMLIMIT_MODERATE;
		case HashType::SLOW: return crypto_pwhash_MEMLIMIT_SENSITIVE;
	}
	assert(false);
	return -1;
}

} // namespace detail

using PasswordView = std::string_view;

class Hash final : public crypto::detail::ByteArray<Hash, char, crypto_pwhash_STRBYTES> {};
using HashRef = util::Span<Hash::value_type, Hash::SIZE>;
using HashView = util::Span<const Hash::value_type, Hash::SIZE>;

class Key final : public crypto::detail::ByteArray<Key, unsigned char, crypto_box_SEEDBYTES> {};
using KeyRef = util::Span<Key::value_type, Key::SIZE>;
using KeyView = util::Span<const Key::value_type, Key::SIZE>;

class Salt final : public crypto::detail::ByteArray<Salt, unsigned char, crypto_pwhash_SALTBYTES> {};
using SaltRef = util::Span<Salt::value_type, Salt::SIZE>;
using SaltView = util::Span<const Salt::value_type, Salt::SIZE>;

[[nodiscard]] inline auto copyHash(HashRef result, std::string_view str) noexcept -> bool {
	if (str.size() >= result.size()) {
		return false;
	}
	util::copy(str, result.begin());
	result[str.size()] = '\0';
	return true;
}

[[nodiscard]] inline auto hash(HashRef result, PasswordView password, HashType type) noexcept -> bool {
	return crypto_pwhash_str(result.data(), password.data(), password.size(), detail::getOpsLimit(type), detail::getMemLimit(type)) == 0;
}

[[nodiscard]] inline auto isHash(HashView hash) noexcept -> bool {
	return crypto_pwhash_str_needs_rehash(hash.data(), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != -1;
}

[[nodiscard]] inline auto isHash(HashView hash, HashType type) noexcept -> bool {
	return crypto_pwhash_str_needs_rehash(hash.data(), detail::getOpsLimit(type), detail::getMemLimit(type)) == 0;
}

[[nodiscard]] inline auto verifyHash(HashView hash, PasswordView password) noexcept -> bool {
	assert(std::find(hash.begin(), hash.end(), '\0') != hash.end());
	return crypto_pwhash_str_verify(hash.data(), password.data(), password.size()) == 0;
}

inline auto generateSalt(SaltRef result) noexcept -> void {
	randombytes_buf(result.data(), result.size());
}

inline auto generateSalt(SaltRef result, SeedView seed) noexcept -> void {
	randombytes_buf_deterministic(result.data(), result.size(), seed.data());
}

[[nodiscard]] inline auto deriveKey(KeyRef result, SaltView salt, PasswordView password, HashType type) noexcept -> bool {
	return crypto_pwhash_argon2id(result.data(),
								  result.size(),
								  password.data(),
								  password.size(),
								  salt.data(),
								  detail::getOpsLimit(type),
								  detail::getMemLimit(type),
								  crypto_pwhash_ALG_ARGON2ID13) == 0;
}

} // namespace pw

// Key exchange library.
namespace kx {

class PublicKey final : public crypto::detail::ByteArray<PublicKey, unsigned char, crypto_kx_PUBLICKEYBYTES> {};
using PublicKeyRef = util::Span<PublicKey::value_type, PublicKey::SIZE>;
using PublicKeyView = util::Span<const PublicKey::value_type, PublicKey::SIZE>;

class SecretKey final : public crypto::detail::ByteArray<SecretKey, unsigned char, crypto_kx_SECRETKEYBYTES> {};
using SecretKeyRef = util::Span<SecretKey::value_type, SecretKey::SIZE>;
using SecretKeyView = util::Span<const SecretKey::value_type, SecretKey::SIZE>;

class SendKey final : public crypto::detail::ByteArray<SendKey, unsigned char, crypto_kx_SESSIONKEYBYTES> {};
using SendKeyRef = util::Span<SendKey::value_type, SendKey::SIZE>;
using SendKeyView = util::Span<const SendKey::value_type, SendKey::SIZE>;

class ReceiveKey final : public crypto::detail::ByteArray<ReceiveKey, unsigned char, crypto_kx_SESSIONKEYBYTES> {};
using ReceiveKeyRef = util::Span<ReceiveKey::value_type, ReceiveKey::SIZE>;
using ReceiveKeyView = util::Span<const ReceiveKey::value_type, ReceiveKey::SIZE>;

static_assert(crypto_secretstream_xchacha20poly1305_KEYBYTES == crypto_kx_SESSIONKEYBYTES);

[[nodiscard]] inline auto generateKeypair(PublicKeyRef publicKey, SecretKeyRef secretKey) noexcept -> bool {
	return crypto_kx_keypair(publicKey.data(), secretKey.data()) == 0;
}

[[nodiscard]] inline auto generateSessionKeysServerSide(ReceiveKeyRef receiveKey, SendKeyRef sendKey, PublicKeyView serverPublicKey,
														SecretKeyView serverSecretKey, PublicKeyView clientPublicKey) noexcept -> bool {
	return crypto_kx_server_session_keys(receiveKey.data(), sendKey.data(), serverPublicKey.data(), serverSecretKey.data(), clientPublicKey.data()) == 0;
}

[[nodiscard]] inline auto generateSessionKeysClientSide(ReceiveKeyRef receiveKey, SendKeyRef sendKey, PublicKeyView clientPublicKey,
														SecretKeyView clientSecretKey, PublicKeyView serverPublicKey) noexcept -> bool {
	return crypto_kx_client_session_keys(receiveKey.data(), sendKey.data(), clientPublicKey.data(), clientSecretKey.data(), serverPublicKey.data()) == 0;
}

} // namespace kx

// Secret stream library.
struct Stream final {
private:
	using State = crypto_secretstream_xchacha20poly1305_state;

public:
	static constexpr auto MESSAGE_ADDED_BYTES = crypto_secretstream_xchacha20poly1305_ABYTES;
	static constexpr auto MAX_MESSAGE_SIZE = std::numeric_limits<std::uint16_t>::max() - MESSAGE_ADDED_BYTES;
	static_assert(MAX_MESSAGE_SIZE + MESSAGE_ADDED_BYTES <= crypto_secretstream_xchacha20poly1305_MESSAGEBYTES_MAX);

	using Header = std::array<unsigned char, crypto_secretstream_xchacha20poly1305_HEADERBYTES>;
	using HeaderRef = util::Span<unsigned char, crypto_secretstream_xchacha20poly1305_HEADERBYTES>;
	using HeaderView = util::Span<unsigned char, crypto_secretstream_xchacha20poly1305_HEADERBYTES>;

	class Send final {
	public:
		[[nodiscard]] auto init(HeaderRef header, kx::SendKeyView sendKey) noexcept -> bool {
			return crypto_secretstream_xchacha20poly1305_init_push(&m_state, header.data(), sendKey.data()) == 0;
		}

		[[nodiscard]] auto push(util::Span<std::byte> output, util::Span<const std::byte> input) noexcept -> bool {
			assert(output.size() == input.size() + MESSAGE_ADDED_BYTES);
			return crypto_secretstream_xchacha20poly1305_push(&m_state,
															  reinterpret_cast<unsigned char*>(output.data()),
															  nullptr,
															  reinterpret_cast<const unsigned char*>(input.data()),
															  input.size(),
															  nullptr,
															  0,
															  0) == 0;
		}

	private:
		State m_state{};
	};

	class Receive final {
	public:
		[[nodiscard]] auto init(HeaderView header, kx::ReceiveKeyView receiveKey) noexcept -> bool {
			return crypto_secretstream_xchacha20poly1305_init_pull(&m_state, header.data(), receiveKey.data()) == 0;
		}

		[[nodiscard]] auto pull(util::Span<std::byte> output, util::Span<const std::byte> input) noexcept -> bool {
			assert(output.size() == input.size() - MESSAGE_ADDED_BYTES);
			return crypto_secretstream_xchacha20poly1305_pull(&m_state,
															  reinterpret_cast<unsigned char*>(output.data()),
															  nullptr,
															  nullptr,
															  reinterpret_cast<const unsigned char*>(input.data()),
															  input.size(),
															  nullptr,
															  0) == 0;
		}

	private:
		State m_state{};
	};
};

} // namespace crypto

template <typename T, typename CharType, std::size_t SIZE>
class std::hash<crypto::detail::ByteArray<T, CharType, SIZE>> {
public:
	auto operator()(const crypto::detail::ByteArray<T, CharType, SIZE>& val) const -> std::size_t {
		return m_hasher(util::CRC32{util::asBytes(util::Span{val})});
	}

private:
	std::hash<util::CRC32> m_hasher{};
};

#endif
