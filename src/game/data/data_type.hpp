#ifndef AF2_DATA_DATA_TYPE_HPP
#define AF2_DATA_DATA_TYPE_HPP

template <typename T, typename ValueT>
class DataType {
public:
	using ValueType = ValueT;

	[[nodiscard]] friend constexpr auto operator==(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_value == rhs.m_value;
	}

	[[nodiscard]] friend constexpr auto operator!=(const T& lhs, const T& rhs) noexcept -> bool {
		return lhs.m_value != rhs.m_value;
	}

	[[nodiscard]] constexpr auto value() const noexcept -> ValueType {
		return m_value;
	}

protected:
	constexpr explicit DataType(ValueType value) noexcept
		: m_value(value) {}

	ValueType m_value;
};

#endif
