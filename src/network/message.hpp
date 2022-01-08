#ifndef AF2_NETWORK_MESSAGE_HPP
#define AF2_NETWORK_MESSAGE_HPP

#include "../utilities/type_list.hpp" // util::TypeList

#include <tuple>       // std::apply, std::tuple, std::tie
#include <type_traits> // std::bool_constant, std::integral_constant, std::is_base_of_v, std::conditional_t

namespace net {

enum class MessageCategory {
	UNRELIABLE,
	RELIABLE,
	SECRET,
};

enum class MessageDirection {
	INPUT,
	OUTPUT,
};

template <typename T>
struct TieInputStreamableBase {
	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, T& a) -> Stream& {
		return stream >> a.tie();
	}
};

template <typename T>
struct TieOutputStreamableBase {
	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const T& a) -> Stream& {
		return stream << a.tie();
	}
};

template <typename T>
struct TieInputOutputStreamableBase
	: TieInputStreamableBase<T>
	, TieOutputStreamableBase<T> {};

template <typename T>
struct UnreliableInputMessage : TieInputStreamableBase<T> {};

template <typename T>
struct UnreliableOutputMessage : TieOutputStreamableBase<T> {};

template <typename T>
struct UnreliableInputOutputMessage : TieInputOutputStreamableBase<T> {};

template <typename T>
struct ReliableInputMessage : TieInputStreamableBase<T> {};

template <typename T>
struct ReliableOutputMessage : TieOutputStreamableBase<T> {};

template <typename T>
struct ReliableInputOutputMessage : TieInputOutputStreamableBase<T> {};

template <typename T>
struct SecretInputMessage : TieInputStreamableBase<T> {};

template <typename T>
struct SecretOutputMessage : TieOutputStreamableBase<T> {};

template <typename T>
struct SecretInputOutputMessage : TieInputOutputStreamableBase<T> {};

template <typename T, MessageDirection DIR>
using TieStreamableBase = std::conditional_t<DIR == MessageDirection::INPUT, //
                                             TieInputStreamableBase<T>,      //
                                             TieOutputStreamableBase<T>>;

template <typename T, MessageDirection DIR>
using UnreliableMessage = std::conditional_t<DIR == MessageDirection::INPUT, //
                                             UnreliableInputMessage<T>,      //
                                             UnreliableOutputMessage<T>>;

template <typename T, MessageDirection DIR>
using ReliableMessage = std::conditional_t<DIR == MessageDirection::INPUT, //
                                           ReliableInputMessage<T>,        //
                                           ReliableOutputMessage<T>>;

template <typename T, MessageDirection DIR>
using SecretMessage = std::conditional_t<DIR == MessageDirection::INPUT, //
                                         SecretInputMessage<T>,          //
                                         SecretOutputMessage<T>>;

template <typename T>
struct is_unreliable_message
	: std::bool_constant<                                     //
		  std::is_base_of_v<UnreliableInputMessage<T>, T> ||  //
		  std::is_base_of_v<UnreliableOutputMessage<T>, T> || //
		  std::is_base_of_v<UnreliableInputOutputMessage<T>, T>> {};

template <typename T>
inline constexpr auto is_unreliable_message_v = is_unreliable_message<T>::value;

template <typename T>
struct is_reliable_message
	: std::bool_constant<                                   //
		  std::is_base_of_v<ReliableInputMessage<T>, T> ||  //
		  std::is_base_of_v<ReliableOutputMessage<T>, T> || //
		  std::is_base_of_v<ReliableInputOutputMessage<T>, T>> {};

template <typename T>
inline constexpr auto is_reliable_message_v = is_reliable_message<T>::value;

template <typename T>
struct is_secret_message
	: std::bool_constant<                                 //
		  std::is_base_of_v<SecretInputMessage<T>, T> ||  //
		  std::is_base_of_v<SecretOutputMessage<T>, T> || //
		  std::is_base_of_v<SecretInputOutputMessage<T>, T>> {};

template <typename T>
inline constexpr auto is_secret_message_v = is_secret_message<T>::value;

template <typename T>
struct is_input_message
	: std::bool_constant<                                    //
		  std::is_base_of_v<UnreliableInputMessage<T>, T> || //
		  std::is_base_of_v<ReliableInputMessage<T>, T> ||   //
		  std::is_base_of_v<SecretInputMessage<T>, T>> {};

template <typename T>
inline constexpr auto is_input_message_v = is_input_message<T>::value;

template <typename T>
struct is_output_message
	: std::bool_constant<                                     //
		  std::is_base_of_v<UnreliableOutputMessage<T>, T> || //
		  std::is_base_of_v<ReliableOutputMessage<T>, T> ||   //
		  std::is_base_of_v<SecretOutputMessage<T>, T>> {};

template <typename T>
inline constexpr auto is_output_message_v = is_output_message<T>::value;

template <typename T>
struct is_input_output_message
	: std::bool_constant<                                          //
		  std::is_base_of_v<UnreliableInputOutputMessage<T>, T> || //
		  std::is_base_of_v<ReliableInputOutputMessage<T>, T> ||   //
		  std::is_base_of_v<SecretInputOutputMessage<T>, T>> {};

template <typename T>
inline constexpr auto is_input_output_message_v = is_input_output_message<T>::value;

template <typename T>
struct is_message
	: std::bool_constant<               //
		  is_unreliable_message_v<T> || //
		  is_reliable_message_v<T> ||   //
		  is_secret_message_v<T>> {};

template <typename T>
inline constexpr auto is_message_v = is_message<T>::value;

template <typename Message>
struct message_category_of
	: std::integral_constant<MessageCategory,                       //
                             (is_secret_message_v<Message>) ?       //
                                 MessageCategory::SECRET :          //
                                 (is_reliable_message_v<Message>) ? //
                                     MessageCategory::RELIABLE :    //
                                     MessageCategory::UNRELIABLE> {};

template <typename Message>
inline constexpr auto message_category_of_v = message_category_of<Message>::value;

template <typename Message>
struct message_direction_of
	: std::integral_constant<MessageDirection,               //
                             (is_input_message_v<Message>) ? //
                                 MessageDirection::INPUT :   //
                                 MessageDirection::OUTPUT> {};

template <typename Message>
inline constexpr auto message_direction_of_v = message_direction_of<Message>::value;

template <typename TypeList>
struct is_all_input_messages;

template <typename... Ts>
struct is_all_input_messages<util::TypeList<Ts...>> : std::bool_constant<(is_input_message_v<Ts> && ...)> {};

template <typename TypeList>
inline constexpr auto is_all_input_messages_v = is_all_input_messages<TypeList>::value;

template <typename TypeList>
struct is_all_output_messages;

template <typename... Ts>
struct is_all_output_messages<util::TypeList<Ts...>> : std::bool_constant<(is_output_message_v<Ts> && ...)> {};

template <typename TypeList>
inline constexpr auto is_all_output_messages_v = is_all_output_messages<TypeList>::value;

} // namespace net

#endif
